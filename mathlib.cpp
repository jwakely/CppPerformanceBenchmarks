/*
    Copyright 2007-2010 Adobe Systems Incorporated
    Copyright 2018 Chris Cox
    Distributed under the MIT License (see accompanying file LICENSE_1_0_0.txt
    or a copy at http://stlab.adobe.com/licenses.html )


Goal: Examine performance of the standard math library routines.


Assumptions:

    1) mathlib functions will be optimized.
    
    2) Trivial mathlib functions are implemented as fast macros.



TODO:

//double nan( const char * );    // can't easily test performance of this!

double fma( double, double, double );
double scalbn( double, int );
double scalbln( double, long int );
double nextafter( double, double );
double fdim( double, double );
double remquo( double, double, int * );
double modf( double, double * );

float fmaf( float, float, float );
float scalbnf( float, int );
float scalblnf( float, long int );
float nextafterf( float, float );
float fdimf( float, float );
float remquof( float, float, int * );
float modff( float, float * );

exp10?

*/

/******************************************************************************/

#include "benchmark_stdint.hpp"
#include <cstddef>
#include <cstdio>
#include <ctime>
#include <cstdlib>
#include <cmath>
#include "benchmark_results.h"
#include "benchmark_timer.h"

/******************************************************************************/

// this constant may need to be adjusted to give reasonable minimum times
// For best results, times should be about 1.0 seconds for the minimum test run
int iterations = 50000;


// 8000 items, or between 32k and 64k of data
// this is intended to remain within the L2 cache of most common CPUs
#define SIZE     8000


// initial value for filling our arrays, may be changed from the command line
double init_value = 0.989796;

/******************************************************************************/

// our global arrays of numbers to be operated upon

double dataDouble[SIZE];
float dataFloat[SIZE];

/******************************************************************************/

#include "benchmark_shared_tests.h"

/******************************************************************************/
/******************************************************************************/

// this does not handle NaN
template<typename T>
    inline const T& cheap_min(const T& __a, const T& __b)
    {  return __b < __a ? __b : __a; }

// this does not handle NaN
template<typename T>
    inline const T& cheap_max(const T& __a, const T& __b)
    { return  __a < __b ? __b : __a; }

template<typename T>
    inline int cheap_signbit(const T& __a)
    { return  __a < T(0) ? 1 : 0; }

template<typename T>
    inline const T cheap_hypot(const T& __a, const T& __b)
    {  return sqrt(__a*__a + __b*__b); }

template<typename T>
    inline const T cheap_hypotf(const T& __a, const T& __b)
    {  return sqrtf(__a*__a + __b*__b); }

template<typename T>
    inline const bool cheap_isnan(const T& __a)
    {  return (__a != __a); }

template<typename T>
    inline const bool cheap_isinf(const T& __a)
    {  return (fabs(__a) == INFINITY); }

template<typename T>
    inline const bool cheap_isfinite(const T& __a)
    {  return ((__a == __a) && fabs(__a) != INFINITY); }

/******************************************************************************/
/******************************************************************************/

template <typename T>
    struct mathlib_fabs {
      static T do_shift(T input) { return (fabs(input)); }
    };

/******************************************************************************/

template <typename T>
    struct mathlib_ceil {
      static T do_shift(T input) { return (ceil(input)); }
    };

/******************************************************************************/

template <typename T>
    struct mathlib_floor {
      static T do_shift(T input) { return (floor(input)); }
    };

/******************************************************************************/

template <typename T>
    struct inline_max {
      static T do_shift(T input, T v1) { return (cheap_max(input,v1)); }
    };

/******************************************************************************/

template <typename T>
    struct inline_min {
      static T do_shift(T input, T v1) { return (cheap_min(input,v1)); }
    };

/******************************************************************************/

template <typename T>
    struct mathlib_fmax {
      static T do_shift(T input, T v1) { return (fmax(input,v1)); }
    };

/******************************************************************************/

template <typename T>
    struct mathlib_fmin {
      static T do_shift(T input, T v1) { return (fmin(input,v1)); }
    };

/******************************************************************************/

template <typename T>
    struct mathlib_fpclassify {
      static int do_shift(T input) { return (std::fpclassify(input)); }
    };

/******************************************************************************/

template <typename T>
    struct mathlib_isnormal {
      static int do_shift(T input) { return (std::isnormal(input)); }
    };

/******************************************************************************/

template <typename T>
    struct inline_isfinite {
      static int do_shift(T input) { return (cheap_isfinite(input)); }
    };


/******************************************************************************/

template <typename T>
    struct mathlib_isfinite {
      static int do_shift(T input) { return (std::isfinite(input)); }
    };

/******************************************************************************/

template <typename T>
    struct inline_isinf {
      static int do_shift(T input) { return (cheap_isinf(input)); }
    };

/******************************************************************************/

template <typename T>
    struct mathlib_isinf {
      static int do_shift(T input) { return (std::isinf(input)); }
    };

/******************************************************************************/

template <typename T>
    struct inline_isnan {
      static int do_shift(T input) { return (cheap_isnan(input)); }
    };

/******************************************************************************/

template <typename T>
    struct mathlib_isnan {
      static int do_shift(T input) { return (std::isnan(input)); }
    };

/******************************************************************************/

template <typename T>
    struct inline_signbit {
      static int do_shift(T input) { return (cheap_signbit(input)); }
    };

/******************************************************************************/

template <typename T>
    struct mathlib_signbit {
      static int do_shift(T input) { return (std::signbit(input)); }
    };

/******************************************************************************/

template <typename T>
    struct mathlib_sqrt {
      static T do_shift(T input) { return (sqrt(input)); }
    };

/******************************************************************************/

template <typename T>
    struct inline_hypot {
      static T do_shift(T input, T v1) { return (cheap_hypot(input,v1)); }
    };

/******************************************************************************/

template <typename T>
    struct mathlib_hypot {
      static T do_shift(T input, T v1) { return (hypot(input,v1)); }
    };

/******************************************************************************/

template <typename T>
    struct mathlib_cos {
      static T do_shift(T input) { return (cos(input)); }
    };

/******************************************************************************/

template <typename T>
    struct mathlib_sin {
      static T do_shift(T input) { return (sin(input)); }
    };

/******************************************************************************/

template <typename T>
    struct mathlib_tan {
      static T do_shift(T input) { return (tan(input)); }
    };

/******************************************************************************/

template <typename T>
    struct mathlib_acos {
      static T do_shift(T input) { return (acos(input)); }
    };

/******************************************************************************/

template <typename T>
    struct mathlib_asin {
      static T do_shift(T input) { return (asin(input)); }
    };

/******************************************************************************/

template <typename T>
    struct mathlib_atan {
      static T do_shift(T input) { return (atan(input)); }
    };

/******************************************************************************/

template <typename T>
    struct mathlib_atan2 {
      static T do_shift(T input, T v1) { return (atan2(input,v1)); }
    };

/******************************************************************************/

template <typename T>
    struct mathlib_exp {
      static T do_shift(T input) { return (exp(input)); }
    };

/******************************************************************************/

template <typename T>
    struct mathlib_exp2 {
      static T do_shift(T input) { return (exp2(input)); }
    };

/******************************************************************************/

template <typename T>
    struct mathlib_log {
      static T do_shift(T input) { return (log(input)); }
    };

/******************************************************************************/

template <typename T>
    struct mathlib_log10 {
      static T do_shift(T input) { return (log10(input)); }
    };

/******************************************************************************/

template <typename T>
    struct mathlib_log2 {
      static T do_shift(T input) { return (log2(input)); }
    };

/******************************************************************************/

template <typename T>
    struct mathlib_pow {
      static T do_shift(T input, T v1) { return (pow(input,v1)); }
    };

/******************************************************************************/

template <typename T>
    struct mathlib_round {
      static T do_shift(T input) { return (round(input)); }
    };

/******************************************************************************/

template <typename T>
    struct mathlib_lround {
      static int do_shift(T input) { return (lround(input)); }
    };

/******************************************************************************/

template <typename T>
    struct mathlib_llround {
      static long long int do_shift(T input) { return (llround(input)); }
    };

/******************************************************************************/

template <typename T>
    struct mathlib_trunc {
      static double do_shift(T input) { return (trunc(input)); }
    };

/******************************************************************************/

template <typename T>
    struct mathlib_cosh {
      static double do_shift(T input) { return (cosh(input)); }
    };

/******************************************************************************/

template <typename T>
    struct mathlib_sinh {
      static double do_shift(T input) { return (sinh(input)); }
    };

/******************************************************************************/

template <typename T>
    struct mathlib_tanh {
      static double do_shift(T input) { return (tanh(input)); }
    };

/******************************************************************************/

template <typename T>
    struct mathlib_acosh {
      static double do_shift(T input) { return (acosh(input)); }
    };

/******************************************************************************/

template <typename T>
    struct mathlib_asinh {
      static double do_shift(T input) { return (asinh(input)); }
    };

/******************************************************************************/

template <typename T>
    struct mathlib_atanh {
      static double do_shift(T input) { return (atanh(input)); }
    };

/******************************************************************************/

template <typename T>
    struct mathlib_fmod {
      static T do_shift(T input, T v1) { return (fmod(input,v1)); }
    };

/******************************************************************************/

template <typename T>
    struct mathlib_remainder {
      static T do_shift(T input, T v1) { return (remainder(input,v1)); }
    };

/******************************************************************************/

template <typename T>
    struct mathlib_copysign {
      static T do_shift(T input, T v1) { return (copysign(input,v1)); }
    };

/******************************************************************************/

template <typename T, typename T2>
    struct mathlib_ldexp {
      static T do_shift(T input, T2 v1) { return (ldexp(input,v1)); }
    };

/******************************************************************************/

template <typename T, typename T2>
    struct mathlib_frexp {
      static T do_shift(T input, T2 *v1) { return (frexp(input,v1)); }
    };

/******************************************************************************/

template <typename T>
    struct mathlib_isgreater {
      static bool do_shift(T input, T v1) { return (isgreater(input,v1)); }
    };

/******************************************************************************/

template <typename T>
    struct mathlib_isgreaterequal {
      static bool do_shift(T input, T v1) { return (isgreaterequal(input,v1)); }
    };

/******************************************************************************/

template <typename T>
    struct mathlib_isless {
      static bool do_shift(T input, T v1) { return (isless(input,v1)); }
    };

/******************************************************************************/

template <typename T>
    struct mathlib_islessequal {
      static bool do_shift(T input, T v1) { return (islessequal(input,v1)); }
    };

/******************************************************************************/

template <typename T>
    struct mathlib_islessgreater {
      static bool do_shift(T input, T v1) { return (islessgreater(input,v1)); }
    };

/******************************************************************************/

template <typename T>
    struct mathlib_isunordered {
      static bool do_shift(T input, T v1) { return (isunordered(input,v1)); }
    };

/******************************************************************************/

template <typename T>
    struct mathlib_j0 {
      static double do_shift(T input) { return (j0(input)); }
    };

/******************************************************************************/

template <typename T>
    struct mathlib_j1 {
      static double do_shift(T input) { return (j1(input)); }
    };

/******************************************************************************/

template <typename T>
    struct mathlib_y0 {
      static double do_shift(T input) { return (y0(input)); }
    };

/******************************************************************************/

template <typename T>
    struct mathlib_y1 {
      static double do_shift(T input) { return (y1(input)); }
    };

/******************************************************************************/

template <typename T, typename T2>
    struct mathlib_jn {
      static T do_shift(T input, T2 v1) { return (jn(v1,input)); }
    };

/******************************************************************************/

template <typename T, typename T2>
    struct mathlib_yn {
      static T do_shift(T input, T2 v1) { return (yn(v1,input)); }
    };

/******************************************************************************/

template <typename T>
    struct mathlib_expm1 {
      static double do_shift(T input) { return (expm1(input)); }
    };

/******************************************************************************/

template <typename T>
    struct mathlib_log1p {
      static double do_shift(T input) { return (log1p(input)); }
    };

/******************************************************************************/

template <typename T>
    struct mathlib_logb {
      static double do_shift(T input) { return (logb(input)); }
    };

/******************************************************************************/

template <typename T>
    struct mathlib_cbrt {
      static double do_shift(T input) { return (cbrt(input)); }
    };

/******************************************************************************/

template <typename T>
    struct mathlib_erf {
      static double do_shift(T input) { return (erf(input)); }
    };

/******************************************************************************/

template <typename T>
    struct mathlib_erfc {
      static double do_shift(T input) { return (erfc(input)); }
    };

/******************************************************************************/

template <typename T>
    struct mathlib_lgamma {
      static double do_shift(T input) { return (lgamma(input)); }
    };

/******************************************************************************/

template <typename T>
    struct mathlib_tgamma {
      static double do_shift(T input) { return (tgamma(input)); }
    };

/******************************************************************************/

template <typename T>
    struct mathlib_nearbyint {
      static double do_shift(T input) { return (nearbyint(input)); }
    };

/******************************************************************************/

template <typename T>
    struct mathlib_rint {
      static double do_shift(T input) { return (rint(input)); }
    };

/******************************************************************************/

template <typename T>
    struct mathlib_ilogb {
      static int do_shift(T input) { return (ilogb(input)); }
    };

/******************************************************************************/

template <typename T>
    struct mathlib_lrint {
      static long int do_shift(T input) { return (lrint(input)); }
    };

/******************************************************************************/

template <typename T>
    struct mathlib_llrint {
      static long long int do_shift(T input) { return (llrint(input)); }
    };

/******************************************************************************/
/******************************************************************************/

template <typename T>
    struct mathlib_fabsf {
      static T do_shift(T input) { return (fabsf(input)); }
    };

/******************************************************************************/

template <typename T>
    struct mathlib_ceilf {
      static T do_shift(T input) { return (ceilf(input)); }
    };

/******************************************************************************/

template <typename T>
    struct mathlib_floorf {
      static T do_shift(T input) { return (floorf(input)); }
    };

/******************************************************************************/

template <typename T>
    struct mathlib_fmaxf {
      static T do_shift(T input, T v1) { return (fmaxf(input,v1)); }
    };

/******************************************************************************/

template <typename T>
    struct mathlib_fminf {
      static T do_shift(T input, T v1) { return (fminf(input,v1)); }
    };

/******************************************************************************/

template <typename T>
    struct mathlib_sqrtf {
      static T do_shift(T input) { return (sqrtf(input)); }
    };

/******************************************************************************/

template <typename T>
    struct inline_hypotf {
      static T do_shift(T input, T v1) { return (cheap_hypotf(input,v1)); }
    };

/******************************************************************************/

template <typename T>
    struct mathlib_hypotf {
      static T do_shift(T input, T v1) { return (hypotf(input,v1)); }
    };

/******************************************************************************/

template <typename T>
    struct mathlib_cosf {
      static T do_shift(T input) { return (cosf(input)); }
    };

/******************************************************************************/

template <typename T>
    struct mathlib_sinf {
      static T do_shift(T input) { return (sinf(input)); }
    };

/******************************************************************************/

template <typename T>
    struct mathlib_tanf {
      static T do_shift(T input) { return (tanf(input)); }
    };

/******************************************************************************/

template <typename T>
    struct mathlib_acosf {
      static T do_shift(T input) { return (acosf(input)); }
    };

/******************************************************************************/

template <typename T>
    struct mathlib_asinf {
      static T do_shift(T input) { return (asinf(input)); }
    };

/******************************************************************************/

template <typename T>
    struct mathlib_atanf {
      static T do_shift(T input) { return (atanf(input)); }
    };

/******************************************************************************/

template <typename T>
    struct mathlib_atan2f {
      static T do_shift(T input, T v1) { return (atan2f(input,v1)); }
    };

/******************************************************************************/

template <typename T>
    struct mathlib_expf {
      static T do_shift(T input) { return (expf(input)); }
    };

/******************************************************************************/

template <typename T>
    struct mathlib_exp2f {
      static T do_shift(T input) { return (exp2f(input)); }
    };

/******************************************************************************/

template <typename T>
    struct mathlib_logf {
      static T do_shift(T input) { return (logf(input)); }
    };

/******************************************************************************/

template <typename T>
    struct mathlib_log10f {
      static T do_shift(T input) { return (log10f(input)); }
    };

/******************************************************************************/

template <typename T>
    struct mathlib_log2f {
      static T do_shift(T input) { return (log2f(input)); }
    };

/******************************************************************************/

template <typename T>
    struct mathlib_powf {
      static T do_shift(T input, T v1) { return (powf(input,v1)); }
    };

/******************************************************************************/

template <typename T>
    struct mathlib_roundf {
      static T do_shift(T input) { return (roundf(input)); }
    };

/******************************************************************************/

template <typename T>
    struct mathlib_lroundf {
      static int do_shift(T input) { return (lroundf(input)); }
    };

/******************************************************************************/

template <typename T>
    struct mathlib_llroundf {
      static long long int do_shift(T input) { return (llroundf(input)); }
    };

/******************************************************************************/

template <typename T>
    struct mathlib_truncf {
      static double do_shift(T input) { return (truncf(input)); }
    };

/******************************************************************************/

template <typename T>
    struct mathlib_coshf {
      static double do_shift(T input) { return (coshf(input)); }
    };

/******************************************************************************/

template <typename T>
    struct mathlib_sinhf {
      static double do_shift(T input) { return (sinhf(input)); }
    };

/******************************************************************************/

template <typename T>
    struct mathlib_tanhf {
      static double do_shift(T input) { return (tanhf(input)); }
    };

/******************************************************************************/

template <typename T>
    struct mathlib_acoshf {
      static double do_shift(T input) { return (acoshf(input)); }
    };

/******************************************************************************/

template <typename T>
    struct mathlib_asinhf {
      static double do_shift(T input) { return (asinhf(input)); }
    };

/******************************************************************************/

template <typename T>
    struct mathlib_atanhf {
      static double do_shift(T input) { return (atanhf(input)); }
    };

/******************************************************************************/

template <typename T>
    struct mathlib_fmodf {
      static T do_shift(T input, T v1) { return (fmodf(input,v1)); }
    };

/******************************************************************************/

template <typename T>
    struct mathlib_remainderf {
      static T do_shift(T input, T v1) { return (remainderf(input,v1)); }
    };

/******************************************************************************/

template <typename T>
    struct mathlib_copysignf {
      static T do_shift(T input, T v1) { return (copysignf(input,v1)); }
    };

/******************************************************************************/

template <typename T, typename T2>
    struct mathlib_ldexpf {
      static T do_shift(T input, T2 v1) { return (ldexpf(input,v1)); }
    };

/******************************************************************************/

template <typename T, typename T2>
    struct mathlib_frexpf {
      static T do_shift(T input, T2 *v1) { return (frexpf(input,v1)); }
    };

/******************************************************************************/

template <typename T>
    struct mathlib_expm1f {
      static double do_shift(T input) { return (expm1f(input)); }
    };

/******************************************************************************/

template <typename T>
    struct mathlib_log1pf {
      static double do_shift(T input) { return (log1pf(input)); }
    };

/******************************************************************************/

template <typename T>
    struct mathlib_logbf {
      static double do_shift(T input) { return (logbf(input)); }
    };

/******************************************************************************/

template <typename T>
    struct mathlib_cbrtf {
      static double do_shift(T input) { return (cbrtf(input)); }
    };

/******************************************************************************/

template <typename T>
    struct mathlib_erff {
      static double do_shift(T input) { return (erff(input)); }
    };

/******************************************************************************/

template <typename T>
    struct mathlib_erfcf {
      static double do_shift(T input) { return (erfcf(input)); }
    };

/******************************************************************************/

template <typename T>
    struct mathlib_lgammaf {
      static double do_shift(T input) { return (lgammaf(input)); }
    };

/******************************************************************************/

template <typename T>
    struct mathlib_tgammaf {
      static double do_shift(T input) { return (tgammaf(input)); }
    };

/******************************************************************************/

template <typename T>
    struct mathlib_nearbyintf {
      static double do_shift(T input) { return (nearbyintf(input)); }
    };

/******************************************************************************/

template <typename T>
    struct mathlib_rintf {
      static double do_shift(T input) { return (rintf(input)); }
    };

/******************************************************************************/

template <typename T>
    struct mathlib_ilogbf {
      static int do_shift(T input) { return (ilogbf(input)); }
    };

/******************************************************************************/

template <typename T>
    struct mathlib_lrintf {
      static long int do_shift(T input) { return (lrintf(input)); }
    };

/******************************************************************************/

template <typename T>
    struct mathlib_llrintf {
      static long long int do_shift(T input) { return (llrintf(input)); }
    };

/******************************************************************************/
/******************************************************************************/


int main(int argc, char** argv) {
    double temp = 1.010203;
    
    // output command for documentation:
    int i;
    for (i = 0; i < argc; ++i)
        printf("%s ", argv[i] );
    printf("\n");

    if (argc > 1) iterations = atoi(argv[1]);
    if (argc > 2) init_value = (double) atof(argv[2]);
    if (argc > 3) temp = (double)atof(argv[3]);


// double
    ::fill(dataDouble, dataDouble+SIZE, double(init_value));
    double var1Double_1, var1Double_2, var1Double_3, var1Double_4;
    var1Double_1 = double(temp);
    var1Double_2 = var1Double_1 * double(2.0);
    var1Double_3 = var1Double_1 + double(2.0);
    var1Double_4 = var1Double_1 + var1Double_2 / var1Double_3;
    int var1Int = int(5.0*temp);

    test_constant<double, mathlib_fabs<double> >(dataDouble,SIZE,"double fabs");
    test_constant<double, mathlib_ceil<double> >(dataDouble,SIZE,"double ceil");
    test_constant<double, mathlib_floor<double> >(dataDouble,SIZE,"double floor");
    test_variable1<double, inline_max<double> >(dataDouble,SIZE,var1Double_1,"double inline max");
    test_variable1<double, inline_min<double> >(dataDouble,SIZE,var1Double_1,"double inline min");
    test_variable1<double, mathlib_fmax<double> >(dataDouble,SIZE,var1Double_1,"double fmax");
    test_variable1<double, mathlib_fmin<double> >(dataDouble,SIZE,var1Double_1,"double fmin");
    test_constant_result<double, int, mathlib_fpclassify<double> >(dataDouble,SIZE,"double fpclassify");
    test_constant_result<double, int, mathlib_isnormal<double> >(dataDouble,SIZE,"double isnormal");
    test_constant_result<double, int, inline_isfinite<double> >(dataDouble,SIZE,"double inline isfinite");
    test_constant_result<double, int, mathlib_isfinite<double> >(dataDouble,SIZE,"double isfinite");
    test_constant_result<double, int, inline_isinf<double> >(dataDouble,SIZE,"double inline isinf");
    test_constant_result<double, int, mathlib_isinf<double> >(dataDouble,SIZE,"double isinf");
    test_constant_result<double, int, inline_isnan<double> >(dataDouble,SIZE,"double inline isnan");
    test_constant_result<double, int, mathlib_isnan<double> >(dataDouble,SIZE,"double isnan");
    test_constant_result<double, int, inline_signbit<double> >(dataDouble,SIZE,"double inline signbit");
#if !__INTEL_COMPILER
//signbit is missing from Intel math library, is in math.h 10.0
    test_constant_result<double, int, mathlib_signbit<double> >(dataDouble,SIZE,"double signbit");
#endif
    test_constant<double, mathlib_sqrt<double> >(dataDouble,SIZE,"double sqrt");
    test_variable1<double, inline_hypot<double> >(dataDouble,SIZE,var1Double_1,"double inline hypot");
    test_variable1<double, mathlib_hypot<double> >(dataDouble,SIZE,var1Double_1,"double hypot");
    test_constant<double, mathlib_cos<double> >(dataDouble,SIZE,"double cos");
    test_constant<double, mathlib_sin<double> >(dataDouble,SIZE,"double sin");
    test_constant<double, mathlib_tan<double> >(dataDouble,SIZE,"double tan");
    test_constant<double, mathlib_acos<double> >(dataDouble,SIZE,"double acos");
    test_constant<double, mathlib_asin<double> >(dataDouble,SIZE,"double asin");
    test_constant<double, mathlib_atan<double> >(dataDouble,SIZE,"double atan");
    test_variable1<double, mathlib_atan2<double> >(dataDouble,SIZE,var1Double_1,"double atan2");
    test_constant<double, mathlib_exp<double> >(dataDouble,SIZE,"double exp");
    test_constant<double, mathlib_exp2<double> >(dataDouble,SIZE,"double exp2");
    test_constant<double, mathlib_log<double> >(dataDouble,SIZE,"double log");
    test_constant<double, mathlib_log10<double> >(dataDouble,SIZE,"double log10");
    test_constant<double, mathlib_log2<double> >(dataDouble,SIZE,"double log2");
    test_variable1<double, mathlib_pow<double> >(dataDouble,SIZE,var1Double_1,"double pow");

    test_constant<double, mathlib_round<double> >(dataDouble,SIZE,"double round");
    test_constant_result<double, int, mathlib_lround<double> >(dataDouble,SIZE,"double lround");
    test_constant_result<double, long long int, mathlib_llround<double> >(dataDouble,SIZE,"double llround");
    test_constant<double, mathlib_trunc<double> >(dataDouble,SIZE,"double trunc");
    test_constant<double, mathlib_cosh<double> >(dataDouble,SIZE,"double cosh");
    test_constant<double, mathlib_sinh<double> >(dataDouble,SIZE,"double sinh");
    test_constant<double, mathlib_tanh<double> >(dataDouble,SIZE,"double tanh");
    
    init_value += 3;
    ::fill(dataDouble, dataDouble+SIZE, double(init_value));
    test_constant<double, mathlib_acosh<double> >(dataDouble,SIZE,"double acosh");    // input must be >= 1
    test_constant<double, mathlib_asinh<double> >(dataDouble,SIZE,"double asinh");    // input must be >= 1
    init_value -= 3;
    ::fill(dataDouble, dataDouble+SIZE, double(init_value));
    
    test_constant<double, mathlib_atanh<double> >(dataDouble,SIZE,"double atanh");
    test_variable1<double, mathlib_fmod<double> >(dataDouble,SIZE,var1Double_1,"double fmod");
    test_variable1<double, mathlib_remainder<double> >(dataDouble,SIZE,var1Double_1,"double remainder");
    test_variable1<double, mathlib_copysign<double> >(dataDouble,SIZE,var1Double_1,"double copysign");
    test_variable1<double, int, mathlib_ldexp<double, int> >(dataDouble,SIZE,var1Int,"double ldexp");
    test_variable1ptr<double, int, mathlib_frexp<double, int> >(dataDouble,SIZE,var1Int,"double frexp");
    
#if defined(_WIN32) || defined(_MACHTYPES_H_)
// these are missing on Linux - may be controled by C99 flag? __USE_ISOC99
    test_variable_result<double, bool, mathlib_isgreater<double> >(dataDouble,SIZE,var1Double_4,"double isgreater");
    test_variable_result<double, bool, mathlib_isgreaterequal<double> >(dataDouble,SIZE,var1Double_4,"double isgreaterequal");
    test_variable_result<double, bool, mathlib_isless<double> >(dataDouble,SIZE,var1Double_4,"double isless");
    test_variable_result<double, bool, mathlib_islessequal<double> >(dataDouble,SIZE,var1Double_4,"double islessequal");
    test_variable_result<double, bool, mathlib_islessgreater<double> >(dataDouble,SIZE,var1Double_4,"double islessgreater");
    test_variable_result<double, bool, mathlib_isunordered<double> >(dataDouble,SIZE,var1Double_4,"double isunordered");
#endif
    
    test_constant<double, mathlib_j0<double> >(dataDouble,SIZE,"double j0");
    test_constant<double, mathlib_j1<double> >(dataDouble,SIZE,"double j1");
    test_constant<double, mathlib_y0<double> >(dataDouble,SIZE,"double y0");
    test_constant<double, mathlib_y1<double> >(dataDouble,SIZE,"double y1");
    test_variable1<double, int, mathlib_jn<double, int> >(dataDouble,SIZE,var1Int,"double jn");
    test_variable1<double, int, mathlib_yn<double, int> >(dataDouble,SIZE,var1Int,"double yn");
    
    test_constant<double, mathlib_expm1<double> >(dataDouble,SIZE,"double expm1");
    test_constant<double, mathlib_log1p<double> >(dataDouble,SIZE,"double log1p");
    test_constant<double, mathlib_logb<double> >(dataDouble,SIZE,"double logb");
    test_constant<double, mathlib_cbrt<double> >(dataDouble,SIZE,"double cbrt");
    test_constant<double, mathlib_erf<double> >(dataDouble,SIZE,"double erf");
    test_constant<double, mathlib_erfc<double> >(dataDouble,SIZE,"double erfc");
    test_constant<double, mathlib_lgamma<double> >(dataDouble,SIZE,"double lgamma");
    test_constant<double, mathlib_tgamma<double> >(dataDouble,SIZE,"double tgamma");
    test_constant<double, mathlib_nearbyint<double> >(dataDouble,SIZE,"double nearbyint");
    test_constant<double, mathlib_rint<double> >(dataDouble,SIZE,"double rint");
    test_constant_result<double, int, mathlib_ilogb<double> >(dataDouble,SIZE,"double ilogb");
    test_constant_result<double, long int, mathlib_lrint<double> >(dataDouble,SIZE,"double lrint");
    test_constant_result<double, long long int, mathlib_llrint<double> >(dataDouble,SIZE,"double llrint");

    summarize("double mathlib", SIZE, iterations, kDontShowGMeans, kDontShowPenalty );





// float
    ::fill(dataFloat, dataFloat+SIZE, float(init_value));
    float var1Float_1, var1Float_2, var1Float_3, var1Float_4;
    var1Float_1 = float(temp);
    var1Float_2 = var1Float_1 * float(2.0);
    var1Float_3 = var1Float_1 + float(2.0);
    var1Float_4 = var1Float_1 + var1Float_2 / var1Float_3;

    test_constant<float, mathlib_fabsf<float> >(dataFloat,SIZE,"float fabs");
    test_constant<float, mathlib_ceilf<float> >(dataFloat,SIZE,"float ceil");
    test_constant<float, mathlib_floorf<float> >(dataFloat,SIZE,"float floor");
    test_variable1<float, inline_max<float> >(dataFloat,SIZE,var1Float_1,"float inline max");
    test_variable1<float, inline_min<float> >(dataFloat,SIZE,var1Float_1,"float inline min");
    test_variable1<float, mathlib_fmaxf<float> >(dataFloat,SIZE,var1Float_1,"float fmax");
    test_variable1<float, mathlib_fminf<float> >(dataFloat,SIZE,var1Float_1,"float fmin");
    test_constant_result<float, int, mathlib_fpclassify<float> >(dataFloat,SIZE,"float fpclassify");
    test_constant_result<float, int, mathlib_isnormal<float> >(dataFloat,SIZE,"float isnormal");
    test_constant_result<float, int, inline_isfinite<float> >(dataFloat,SIZE,"float inline isfinite");
    test_constant_result<float, int, mathlib_isfinite<float> >(dataFloat,SIZE,"float isfinite");
    test_constant_result<float, int, inline_isinf<float> >(dataFloat,SIZE,"float inline isinf");
    test_constant_result<float, int, mathlib_isinf<float> >(dataFloat,SIZE,"float isinf");
    test_constant_result<float, int, inline_isnan<float> >(dataFloat,SIZE,"float inline isnan");
    test_constant_result<float, int, mathlib_isnan<float> >(dataFloat,SIZE,"float isnan");
    test_constant_result<float, int, inline_signbit<float> >(dataFloat,SIZE,"float inline signbit");
#if !__INTEL_COMPILER
//signbit is missing from Intel math library, is in math.h 10.0
    test_constant_result<float, int, mathlib_signbit<float> >(dataFloat,SIZE,"float signbit");
#endif
    test_constant<float, mathlib_sqrtf<float> >(dataFloat,SIZE,"float sqrt");
    test_variable1<float, inline_hypotf<float> >(dataFloat,SIZE,var1Float_1,"float inline hypot");
    test_variable1<float, mathlib_hypotf<float> >(dataFloat,SIZE,var1Float_1,"float hypot");
    test_constant<float, mathlib_cosf<float> >(dataFloat,SIZE,"float cos");
    test_constant<float, mathlib_sinf<float> >(dataFloat,SIZE,"float sin");
    test_constant<float, mathlib_tanf<float> >(dataFloat,SIZE,"float tan");
    test_constant<float, mathlib_acosf<float> >(dataFloat,SIZE,"float acos");
    test_constant<float, mathlib_asinf<float> >(dataFloat,SIZE,"float asin");
    test_constant<float, mathlib_atanf<float> >(dataFloat,SIZE,"float atan");
    test_variable1<float, mathlib_atan2f<float> >(dataFloat,SIZE,var1Float_1,"float atan2");
    test_constant<float, mathlib_expf<float> >(dataFloat,SIZE,"float exp");
    test_constant<float, mathlib_exp2f<float> >(dataFloat,SIZE,"float exp2");
    test_constant<float, mathlib_logf<float> >(dataFloat,SIZE,"float log");
    test_constant<float, mathlib_log10f<float> >(dataFloat,SIZE,"float log10");
    test_constant<float, mathlib_log2f<float> >(dataFloat,SIZE,"float log2");
    test_variable1<float, mathlib_powf<float> >(dataFloat,SIZE,var1Float_1,"float pow");
    
    test_constant<float, mathlib_roundf<float> >(dataFloat,SIZE,"float round");
    test_constant_result<float, int, mathlib_lroundf<float> >(dataFloat,SIZE,"float lround");
    test_constant_result<float, long long int, mathlib_llroundf<float> >(dataFloat,SIZE,"float llround");
    test_constant<float, mathlib_truncf<float> >(dataFloat,SIZE,"float trunc");
    test_constant<float, mathlib_coshf<float> >(dataFloat,SIZE,"float cosh");
    test_constant<float, mathlib_sinhf<float> >(dataFloat,SIZE,"float sinh");
    test_constant<float, mathlib_tanhf<float> >(dataFloat,SIZE,"float tanh");
    
    init_value += 3;
    ::fill(dataFloat, dataFloat+SIZE, float(init_value));
    test_constant<float, mathlib_acoshf<float> >(dataFloat,SIZE,"float acosh");    // input must be > 1
    test_constant<float, mathlib_asinhf<float> >(dataFloat,SIZE,"float asinh");    // input must be > 1
    init_value -= 3;
    ::fill(dataFloat, dataFloat+SIZE, float(init_value));
    
    test_constant<float, mathlib_atanhf<float> >(dataFloat,SIZE,"float atanh");
    test_variable1<float, mathlib_fmodf<float> >(dataFloat,SIZE,var1Float_1,"float fmod");
    test_variable1<float, mathlib_remainderf<float> >(dataFloat,SIZE,var1Float_1,"float remainder");
    test_variable1<float, mathlib_copysignf<float> >(dataFloat,SIZE,var1Float_1,"float copysign");
    test_variable1<float, int, mathlib_ldexpf<float, int> >(dataFloat,SIZE,var1Int,"float ldexp");
    test_variable1ptr<float, int, mathlib_frexpf<float, int> >(dataFloat,SIZE,var1Int,"float frexp");
    
#if defined(_WIN32) || defined(_MACHTYPES_H_)
// these are missing on Linux - may be controled by C99 flag? __USE_ISOC99
    test_variable_result<float, bool, mathlib_isgreater<float> >(dataFloat,SIZE,var1Float_4,"float isgreater");
    test_variable_result<float, bool, mathlib_isgreaterequal<float> >(dataFloat,SIZE,var1Float_4,"float isgreaterequal");
    test_variable_result<float, bool, mathlib_isless<float> >(dataFloat,SIZE,var1Float_4,"float isless");
    test_variable_result<float, bool, mathlib_islessequal<float> >(dataFloat,SIZE,var1Float_4,"float islessequal");
    test_variable_result<float, bool, mathlib_islessgreater<float> >(dataFloat,SIZE,var1Float_4,"float islessgreater");
    test_variable_result<float, bool, mathlib_isunordered<float> >(dataFloat,SIZE,var1Float_4,"float isunordered");
#endif

    // bessel functions do not have float versions
    test_constant<float, mathlib_expm1f<float> >(dataFloat,SIZE,"float expm1");
    test_constant<float, mathlib_log1pf<float> >(dataFloat,SIZE,"float log1p");
    test_constant<float, mathlib_logbf<float> >(dataFloat,SIZE,"float logb");
    test_constant<float, mathlib_cbrtf<float> >(dataFloat,SIZE,"float cbrt");
    test_constant<float, mathlib_erff<float> >(dataFloat,SIZE,"float erf");
    test_constant<float, mathlib_erfcf<float> >(dataFloat,SIZE,"float erfc");
    test_constant<float, mathlib_lgammaf<float> >(dataFloat,SIZE,"float lgamma");
    test_constant<float, mathlib_tgammaf<float> >(dataFloat,SIZE,"float tgamma");
    test_constant<float, mathlib_nearbyintf<float> >(dataFloat,SIZE,"float nearbyint");
    test_constant<float, mathlib_rintf<float> >(dataFloat,SIZE,"float rint");
    test_constant_result<float, int, mathlib_ilogbf<float> >(dataFloat,SIZE,"float ilogb");
    test_constant_result<float, long int, mathlib_lrintf<float> >(dataFloat,SIZE,"float lrint");
    test_constant_result<float, long long int, mathlib_llrintf<float> >(dataFloat,SIZE,"float llrint");
    
    summarize("float mathlib", SIZE, iterations, kDontShowGMeans, kDontShowPenalty );


    
    return 0;
}

// the end
/******************************************************************************/
/******************************************************************************/
