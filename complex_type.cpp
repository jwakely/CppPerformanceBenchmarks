/*
    Copyright 2007-2008 Adobe Systems Incorporated
    Copyright 2018 Chris Cox
    Distributed under the MIT License (see accompanying file LICENSE_1_0_0.txt
    or a copy at http://stlab.adobe.com/licenses.html )


Goal: Test compiler optimizations related to the std::complex type.


Assumptions:

    1) the compiler will apply constant folding on complex values
        result = input + A + B + C + D    ==>        result = input + (A+B+C+D)
        result = input - A - B - C - D    ==>        result = input - (A+B+C+D)
        result = input * A * B * C * D    ==>        result = input * (A*B*C*D)
        result = ((((input/A) /B) /C) /D)    ==>        result = input / (A*B*C*D)
    
    3) the compiler will move redundant complex calculations out of a loop
        for (i = 0; i < N; ++i)                    temp = A + B + C + D;
            result = input[i] + A+B+C+D;    ==>    for (i = 0; i < N; ++i)
                                                    result = input[i] + temp;
    
    2) the compiler will apply common subexpression elimination on complex values
        result1 = input1 + A * (input1 - input2)        temp = A * (input1 - input2)
        result2 = input2 + A * (input1 - input2)    ==> result1 = input1 + temp;
                                                        result2 = input2 + temp;

    4) the compiler will simplify common algebraic identities with complex variables
        aka algebraic simplification
        input + 0 ==> input
        input - 0 ==> input
        0 - input ==> -input
        -(-input) ==> input
        input * 1 ==> input
        input / 1 ==> input
        input * 0 ==> 0
        input - input ==> 0



TODO - math functions operating on complex values
TODO - profile simple constant folding and check assembly

*/

/******************************************************************************/

#include "benchmark_stdint.hpp"
#include <stddef.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <complex>
#include <deque>
#include "benchmark_results.h"
#include "benchmark_timer.h"
#include "benchmark_typenames.h"

using namespace std;

/******************************************************************************/

// this constant may need to be adjusted to give reasonable minimum times
// For best results, times should be about 1.0 seconds for the minimum test run
int iterations = 300000;


// 8000 items, or between 64k and 256k of data
// this is intended to remain within the L2 cache of most common CPUs
#define SIZE     8000


// initial value for filling our arrays, may be changed from the command line
double init_value = 1.0;

/******************************************************************************/

typedef complex<double>    double_complex;
typedef complex<float>     float_complex;
typedef complex<int32_t>   int32_complex;
typedef complex<int64_t>   int64_complex;

/******************************************************************************/

namespace benchmark {

template<>
std::string getTypeName<float_complex>() { return std::string("float_complex"); }

template<>
std::string getTypeName<double_complex>() { return std::string("double_complex"); }

template<>
std::string getTypeName<int32_complex>() { return std::string("int32_complex"); }

template<>
std::string getTypeName<int64_complex>() { return std::string("int64_complex"); }

};

/******************************************************************************/

#include "benchmark_shared_tests.h"

/******************************************************************************/


template<>
inline bool tolerance_equal(float_complex &a, float_complex &b) {
    float_complex diff = a - b;
    return (abs(diff) < 5);        // complex math in float is really imprecise
}

template<>
inline bool tolerance_equal(double_complex &a, double_complex &b) {
    double_complex diff = a - b;
    return (abs(diff) < 1.0e-6);
}

template<>
inline bool tolerance_equal(int32_complex &a, int32_complex &b) {
    int32_complex diff = a - b;
    return (abs(diff) < 1);
}

template<>
inline bool tolerance_equal(int64_complex &a, int64_complex &b) {
    int64_complex diff = a - b;
    return (abs(diff) < 1);
}

/******************************************************************************/

template <typename T>
    struct complex_constant_add {
      static T do_shift(T input) { return (input + T(1+2+3+4,2+3+4+5)); }
    };

/******************************************************************************/

template <typename T>
    struct complex_multiple_constant_add {
      static T do_shift(T input) { return (input + (T(1,2) + T(2,3) + T(3,4) + T(4,5))); }
    };

/******************************************************************************/

template <typename T>
    struct complex_constant_sub {
      static T do_shift(T input) { return (input - T(1+2+3+4,2+3+4+5)); }
    };

/******************************************************************************/

template <typename T>
    struct complex_multiple_constant_sub {
      static T do_shift(T input) { return (input - (T(1,2) - T(2,3) - T(3,4) - T(4,5))); }
    };

/******************************************************************************/

template <typename T>
    struct complex_constant_multiply {
      static T do_shift(T input) { return (input * T(-185,-180)); }
    };

/******************************************************************************/

template <typename T>
    struct complex_multiple_constant_multiply {
      static T do_shift(T input) { return (input * (T(1,2) * T(2,3) * T(3,4) * T(4,5))); }
    };

/******************************************************************************/

template <typename T>
    struct complex_multiple_constant_multiply2 {
      static T do_shift(T input) { return (input + (T(1,2) * T(2,3) * T(3,4) * T(4,5))); }
    };

/******************************************************************************/

template <typename T>
    struct complex_constant_divide {
      static T do_shift(T input) { return (input / T(2,3)); }
    };

/******************************************************************************/

template <typename T>
    struct complex_multiple_constant_divide {
      static T do_shift(T input) { return (input / ((((T(48,90) / T(2,3)) / T(3,4)) / T(4,5)))); }
    };

/******************************************************************************/

template <typename T>
    struct complex_multiple_constant_divide2 {
      static T do_shift(T input) { return (input + ((((T(48,90) / T(2,3)) / T(3,4)) / T(4,5)))); }
    };

/******************************************************************************/

template <typename T>
    struct complex_multiple_constant_mixed {
      static T do_shift(T input) { return (input + T(2,3) - T(3,4) * T(4,5) / T(5,6)); }
    };

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

template <typename T, typename Shifter>
void test_variable1(T* first, int count, const T v1, const std::string &label) {
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
void test_variable4(T* first, int count, const T v1, const T v2, const T v3, const T v4, const std::string &label) {
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

template <typename T, typename Shifter>
void test_CSE_fullopt(T* first, int count, const T v1, const std::string &label) {
  int i;
  
  start_timer();
  
  // This is as far as most compilers can go in optimizing this function.
  // CSE + algebraic simplification
  for(i = 0; i < iterations; ++i) {
    T result = 0;
    result += first[0] - first[1];
    for (int n = 1; n < count; ++n) {
        result += first[n-1] - first[n];
    }
    check_shifted_variable_sum_CSE<T, Shifter>(result, v1);
  }

  // need the labels to remain valid until we print the summary
  gLabels.push_back( label );
  record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

template <typename T, typename Shifter>
void test_CSE_halfopt(T* first, int count, const T v1, const std::string &label) {
  int i;
  
  start_timer();
  
  // do the CSE by hand
  for(i = 0; i < iterations; ++i) {
    T result = 0;
    T temp = Shifter::do_shift( v1, first[0], first[1] );
    temp += temp;
    result += first[0] + temp;
    result -= first[1] + temp;
    for (int n = 1; n < count; ++n) {
        temp = Shifter::do_shift( v1, first[n-1], first[n] );
        temp += temp;
        result += first[n-1] + temp;
        result -= first[n] + temp;
    }
    check_shifted_variable_sum_CSE<T, Shifter>(result, v1);
  }

  // need the labels to remain valid until we print the summary
  gLabels.push_back( label );
  record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

template <typename T, typename Shifter>
void test_CSE(T* first, int count, const T v1, const std::string &label) {
  int i;
  
  start_timer();
  
  for(i = 0; i < iterations; ++i) {
    T result = 0;
    result += first[0] + Shifter::do_shift( v1, first[0], first[1] ) + Shifter::do_shift( v1, first[0], first[1] );
    result -= first[1] + Shifter::do_shift( v1, first[0], first[1] ) + Shifter::do_shift( v1, first[0], first[1] );
    for (int n = 1; n < count; ++n) {
        result += first[n-1] + Shifter::do_shift( v1, first[n-1], first[n] ) + Shifter::do_shift( v1, first[n-1], first[n] );
        result -= first[n] + Shifter::do_shift( v1, first[n-1], first[n] ) + Shifter::do_shift( v1, first[n-1], first[n] );
    }
    check_shifted_variable_sum_CSE<T, Shifter>(result, v1);
  }
  
  // need the labels to remain valid until we print the summary
  gLabels.push_back( label );
  record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

// this expansion exposes problems in several compilers
template <typename T, typename Shifter>
void test_CSE2(T* first, int count, const T v1, const std::string &label) {
  int i;
  
  start_timer();
  
  for(i = 0; i < iterations; ++i) {
    T result = 0;
    result += first[0] + Shifter::do_shift( v1, first[0], first[1] ) + Shifter::do_shift( v1, first[0], first[1] );
    result -= first[1] + Shifter::do_shift( v1, first[0], first[1] ) + Shifter::do_shift( v1, first[0], first[1] );
    result += first[0] + Shifter::do_shift( v1, first[0], first[1] ) + Shifter::do_shift( v1, first[0], first[1] );
    result -= first[1] + Shifter::do_shift( v1, first[0], first[1] ) + Shifter::do_shift( v1, first[0], first[1] );
    for (int n = 1; n < count; ++n) {
        result += first[n-1] + Shifter::do_shift( v1, first[n-1], first[n] ) + Shifter::do_shift( v1, first[n-1], first[n] );
        result -= first[n] + Shifter::do_shift( v1, first[n-1], first[n] ) + Shifter::do_shift( v1, first[n-1], first[n] );
        result += first[n-1] + Shifter::do_shift( v1, first[n-1], first[n] ) + Shifter::do_shift( v1, first[n-1], first[n] );
        result -= first[n] + Shifter::do_shift( v1, first[n-1], first[n] ) + Shifter::do_shift( v1, first[n-1], first[n] );
    }
    check_shifted_variable_sum_CSE<T, Shifter>(result, v1);
  }
  
  // need the labels to remain valid until we print the summary
  gLabels.push_back( label );
  record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

template< typename T >
void testComplexType(double temp)
{
    T data[ SIZE ];
    
    T var1 = T(temp,temp+1);
    T var2 = var1 * T(2.0,3.0);
    T var3 = var1 + T(2.0,4.0);
    T var4 = var1 + var2 / var3;

    ::fill(data, data+SIZE, T(init_value,0) );
    
    std::string myTypeName( getTypeName<T>() );
    gLabels.clear();



    // test constant folding (could also be handled by CSE or loop invariant code motion)
    test_constant<T, custom_two<T> >(data,SIZE,myTypeName + " constant");
    test_constant<T, custom_add_constants<T> >(data,SIZE,myTypeName + " add constants");
    test_constant<T, custom_sub_constants<T> >(data,SIZE,myTypeName + " subtract constants");
    test_constant<T, custom_multiply_constants<T> >(data,SIZE,myTypeName + " multiply constants");
    test_constant<T, custom_divide_constants<T> >(data,SIZE,myTypeName + " divide constants");
    test_constant<T, custom_equal_constants<T> >(data,SIZE,myTypeName + " equal constants");
    test_constant<T, custom_notequal_constants<T> >(data,SIZE,myTypeName + " notequal constants");
    
    std::string temp6( myTypeName + " simple constant folding" );
    summarize( temp6.c_str(), SIZE, iterations, kDontShowGMeans, kDontShowPenalty);
    
    
    test_constant<T, complex_constant_add<T> >(data,SIZE,myTypeName + " constant add");
    test_constant<T, complex_multiple_constant_add<T> >(data,SIZE,myTypeName + " multiple constant adds");
    test_constant<T, complex_constant_sub<T> >(data,SIZE,myTypeName + " constant subtract");
    test_constant<T, complex_multiple_constant_sub<T> >(data,SIZE,myTypeName + " multiple constant subtracts");
    test_constant<T, complex_constant_multiply<T> >(data,SIZE,myTypeName + " constant multiply");
    test_constant<T, complex_multiple_constant_multiply<T> >(data,SIZE,myTypeName + " multiple constant multiplies");
    test_constant<T, complex_multiple_constant_multiply2<T> >(data,SIZE,myTypeName + " multiple constant multiplies2");
    test_constant<T, complex_constant_divide<T> >(data,SIZE,myTypeName + " constant divide");
    test_constant<T, complex_multiple_constant_divide<T> >(data,SIZE,myTypeName + " multiple constant divides");
    test_constant<T, complex_multiple_constant_divide2<T> >(data,SIZE,myTypeName + " multiple constant divides2");
    test_constant<T, complex_multiple_constant_mixed<T> >(data,SIZE,myTypeName + " multiple constant mixed");
    
    std::string temp1( myTypeName + " constant folding" );
    summarize( temp1.c_str(), SIZE, iterations, kDontShowGMeans, kDontShowPenalty);


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
    test_variable4< T, custom_divide_multiple_variable<T> > (data, SIZE, var1, var2, var3, var4, myTypeName + " multiple variable divides");
    test_variable4< T, custom_divide_multiple_variable2<T> > (data, SIZE, var1, var2, var3, var4, myTypeName + " multiple variable divides2");
    test_variable4< T, custom_mixed_multiple_variable<T> > (data, SIZE, var1, var2, var3, var4, myTypeName + " multiple variable mixed");
    test_variable4< T, custom_mixed_multiple_variable2<T> > (data, SIZE, var1, var2, var3, var4, myTypeName + " multiple variable mixed2");
    
    std::string temp2( myTypeName + " loop invariants" );
    summarize( temp2.c_str(), SIZE, iterations, kDontShowGMeans, kDontShowPenalty);
    
    
    // test common subexpression elimination
    test_CSE_fullopt< T, custom_cse1<T> > (data, SIZE, var1, myTypeName + " CSE opt");
    test_CSE_halfopt< T, custom_cse1<T> > (data, SIZE, var1, myTypeName + " CSE half opt");
    test_CSE< T, custom_cse1<T> > (data, SIZE, var1, myTypeName + " CSE");
    test_CSE2< T, custom_cse1<T> > (data, SIZE, var1, myTypeName + " CSE2X");

    std::string temp3( myTypeName + " CSE" );
    summarize( temp3.c_str(), SIZE, iterations, kDontShowGMeans, kDontShowPenalty);

    
    // test algebraic simplification
    test_constant<T, custom_identity<T> >(data,SIZE,myTypeName + " copy");
    test_constant<T, custom_add_zero<T> >(data,SIZE,myTypeName + " add zero");
    test_constant<T, custom_sub_zero<T> >(data,SIZE,myTypeName + " subtract zero");
    test_constant<T, custom_negate<T> >(data,SIZE,myTypeName + " negate");
    test_constant<T, custom_negate_twice<T> >(data,SIZE,myTypeName + " negate twice");
    test_constant<T, custom_zero_minus<T> >(data,SIZE,myTypeName + " zero minus");
    test_constant<T, custom_times_one<T> >(data,SIZE,myTypeName + " times one");
    test_constant<T, custom_divideby_one<T> >(data,SIZE,myTypeName + " divide by one");
    test_constant<T, custom_algebra_mixed<T> >(data,SIZE,myTypeName + " mixed algebra");
    
    std::string temp4( myTypeName + " algebraic simplification" );
    summarize( temp4.c_str(), SIZE, iterations, kDontShowGMeans, kDontShowPenalty );
    
    
    test_constant<T, custom_zero<T> >(data,SIZE,myTypeName + " zero");
    test_constant<T, custom_times_zero<T> >(data,SIZE,myTypeName + " times zero");
    test_constant<T, custom_subtract_self<T> >(data,SIZE,myTypeName + " subtract self");
    test_constant<T, custom_algebra_mixed_constant<T> >(data,SIZE,myTypeName + " mixed constant");
    
    std::string temp5( myTypeName + " algebraic simplification to constant" );
    summarize( temp5.c_str(), SIZE, iterations, kDontShowGMeans, kDontShowPenalty );


}

/******************************************************************************/
/******************************************************************************/

int main(int argc, char** argv) {
    // temp value for variables
    double temp = 1.0;
    
    // output command for documentation:
    int i;
    for (i = 0; i < argc; ++i)
        printf("%s ", argv[i] );
    printf("\n");

    if (argc > 1) iterations = atoi(argv[1]);
    if (argc > 2) init_value = (double) atof(argv[2]);
    if (argc > 3) temp = (double)atof(argv[3]);


    // shorter integer types don't make much sense
    testComplexType<int32_complex>(temp);
    testComplexType<int64_complex>(temp);
    
    // these are slower
    iterations /= 2;
    testComplexType<float_complex>(temp);
    testComplexType<double_complex>(temp);


    return 0;
}

// the end
/******************************************************************************/
/******************************************************************************/
