/*
    Copyright 2008 Adobe Systems Incorporated
    Copyright 2018-2019 Chris Cox
    Distributed under the MIT License (see accompanying file LICENSE_1_0_0.txt
    or a copy at http://stlab.adobe.com/licenses.html )


Goal: Test compiler optimizations related to propagation of constant simple language defined types.

Assumptions:

    1) The compiler will propagate constant values through expressions to simplify them.
        aka: constant propagation.

    2) The compiler will propagate constant values through template parameters to simplify them.

    3) The compiler will propagate constant values through function call parameters to simplify them.

    4) The compiler will recognize unchanged global values as constants.

    5) The compiler will recognize unchanged static values as constants.

    6) The compiler will propagate constant values through all layers of function calls to simplify them.

    7) The compiler will propagate constant values through all layers of template parameters to simplify them.


NOTE - This also hits loop invariant code motion in many cases,
        but even those show up as slow compared to adding a constant.

NOTE - If the optimization is done correctly, then even the check_sum can be hoisted out of the loops,
       and the entire iterations/printf branched around or removed.
 
*/

/******************************************************************************/

#include "benchmark_stdint.hpp"
#include <stddef.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <math.h>
#include <algorithm>
#include <string>
#include <deque>
#include "benchmark_results.h"
#include "benchmark_timer.h"
#include "benchmark_typenames.h"

/******************************************************************************/

// this constant may need to be adjusted to give reasonable minimum times
// For best results, times should be about 1.0 seconds for the minimum test run
int iterations = 9000000;


// 8000 items, or between 8k and 64k of data
// this is intended to remain within the L2 cache of most common CPUs
#define SIZE     8000


// initial value for filling our arrays, may be changed from the command line
double init_value = 4.0;

/******************************************************************************/

#include "benchmark_shared_tests.h"

/******************************************************************************/

template <typename T, typename Shifter>
inline void check_shifted_sum(T result, const std::string &label) {
    T temp = (T)SIZE * Shifter::do_shift((T)init_value);
    if (!tolerance_equal<T>(result,temp))
        printf("test %s failed\n", label.c_str());
}

/******************************************************************************/

static std::deque<std::string> gLabels;

template <typename T, typename Shifter>
void test_constant(T* first, int count, const std::string label) {
  int i;
  
  start_timer();
  
  for(i = 0; i < iterations; ++i) {
    T result = 0;
    for (int n = 0; n < count; ++n) {
        result += Shifter::do_shift( first[n] );
    }
    check_shifted_sum<T, Shifter>(result, label );
  }

  // need the labels to remain valid until we print the summary
  gLabels.push_back( label );
  record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

// TODO - ccox - move these into shared tests!

// super trivial, constants defined inline
template <typename T>
    struct custom_inline1 {
      static T do_shift(T input) {
        return (T(12) / (T(2) * (T(3) + T(0))));
        }
    };

/******************************************************************************/

// trivial, constants defined in same basic block
template <typename T>
    struct custom_prop1 {
      static T do_shift(T input) {
        T x = T(2);
        T y = T(3);
        T z = T(12);
        T k = T(0);
        return (z / (x * (y + k)));
        }
    };

/******************************************************************************/

// really trivial, constants defined in same basic block
template <typename T>
    struct custom_prop2 {
      static T do_shift(T input) {
        const T x = T(2);
        const T y = T(3);
        const T z = T(12);
        const T k = T(0);
        return (z / (x * (y + k)));
        }
    };

/******************************************************************************/

// really trivial, constants defined in same basic block
template <typename T>
    struct custom_prop3 {
      static T do_shift(T input) {
        static T x = T(2);
        static T y = T(3);
        static T z = T(12);
        static T k = T(0);
        return (z / (x * (y + k)));
        }
    };

/******************************************************************************/

// trivial, constants are passed in as template parameters
template <typename T, T x, T y, T z, T k >
    struct custom_prop1_template_arg {
      static T do_shift(T input) { return (z / (x * (y + k))); }
    };

/******************************************************************************/

// constants are passed in by calling function
template <typename T>
    struct custom_prop1_arg {
      static T do_shift(T input, T x, T y, T z, T k) { return (z / (x * (y + k))); }
    };

/******************************************************************************/

template <typename T, typename Shifter>
inline void check_shifted_sum_arg4(T result, const std::string &label) {
  T temp = (T)SIZE * Shifter::do_shift((T)init_value, 2, 3, 12, 0);
  if (!tolerance_equal<T>(result,temp))
    printf("test %s failed\n", label.c_str());
}

/******************************************************************************/

template <typename T, typename Shifter>
void test_constant_arg4(T* first, int count, const std::string label) {
    int i;

    start_timer();

    for(i = 0; i < iterations; ++i) {
        T result = 0;
        for (int n = 0; n < count; ++n) {
            result += Shifter::do_shift( first[n], 2, 3, 12, 0 );
        }
        check_shifted_sum_arg4<T, Shifter>(result, label);
    }

  // need the labels to remain valid until we print the summary
  gLabels.push_back( label );
  record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

// These global variables are unused anywhere outside this template.
// But many compilers don't check to see if they are constant.
int32_t gX = 2;
int32_t gY = 3;
int32_t gZ = 12;
int32_t gK = 0;

template <typename T, typename Shifter>
void test_constant_arg4_global(T* first, int count, const std::string label) {
    int i;

    start_timer();

    for(i = 0; i < iterations; ++i) {
        T result = 0;
        for (int n = 0; n < count; ++n) {
            result += Shifter::do_shift( first[n], T(gX), T(gY), T(gZ), T(gK) );
        }
        check_shifted_sum_arg4<T, Shifter>(result, label);
    }

  // need the labels to remain valid until we print the summary
  gLabels.push_back( label );
  record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

// These global variables are unused anywhere outside this template,
// Most compilers should be able to propagate the values.
const int32_t gCX = 2;
const int32_t gCY = 3;
const int32_t gCZ = 12;
const int32_t gCK = 0;

template <typename T, typename Shifter>
void test_constant_arg4_global_const(T* first, int count, const std::string label) {
    int i;

    start_timer();

    for(i = 0; i < iterations; ++i) {
        T result = 0;
        for (int n = 0; n < count; ++n) {
            result += Shifter::do_shift( first[n], T(gCX), T(gCY), T(gCZ), T(gCK) );
        }
        check_shifted_sum_arg4<T, Shifter>(result, label);
    }

  // need the labels to remain valid until we print the summary
  gLabels.push_back( label );
  record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

// these global static variables are unused anywhere outside this template
// but many compilers don't check to see if they are constant
static int32_t gSX = 2;
static int32_t gSY = 3;
static int32_t gSZ = 12;
static int32_t gSK = 0;

template <typename T, typename Shifter>
void test_constant_arg4_global_static(T* first, int count, const std::string label) {
    int i;

    start_timer();

    for(i = 0; i < iterations; ++i) {
        T result = 0;
        for (int n = 0; n < count; ++n) {
            result += Shifter::do_shift( first[n], T(gSX), T(gSY), T(gSZ), T(gSK) );
        }
        check_shifted_sum_arg4<T, Shifter>(result, label);
    }

  // need the labels to remain valid until we print the summary
  gLabels.push_back( label );
  record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

// These are staticly defined values, unmodified anywhere in the application.
// Some compilers see them as constant, and some don't.
// Typically, the static declaration is a mistake.

template <typename T, typename Shifter>
void test_constant_arg4_static(T* first, int count, const std::string label) {
    int i;
    
    static int32_t sX = 2;
    static int32_t sY = 3;
    static int32_t sZ = 12;
    static int32_t sK = 0;

    start_timer();

    for(i = 0; i < iterations; ++i) {
        T result = 0;
        for (int n = 0; n < count; ++n) {
            result += Shifter::do_shift( first[n], T(sX), T(sY), T(sZ), T(sK) );
        }
        check_shifted_sum_arg4<T, Shifter>(result, label);
    }

  // need the labels to remain valid until we print the summary
  gLabels.push_back( label );
  record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

template <typename T, typename Shifter>
inline void check_shifted_sum_arg42(T result, T x, T y, T z, T k, const std::string &label) {
  T temp = (T)SIZE * Shifter::do_shift((T)init_value, x, y, z, k);
  if (!tolerance_equal<T>(result,temp))
    printf("test %s failed\n", label.c_str());
}

/******************************************************************************/

// constants defined in outer function
template <typename T, typename Shifter>
void test_constant_arg42(T* first, int count, const std::string label) {
    int i;
  
    start_timer();

    T x = T(2);
    T y = T(3);
    T z = T(12);
    T k = T(0);

    for(i = 0; i < iterations; ++i) {
        T result = 0;
        for (int n = 0; n < count; ++n) {
            result += Shifter::do_shift( first[n], x, y, z, k );
        }
        check_shifted_sum_arg42<T, Shifter>(result, x, y, z, k, label);
    }

  // need the labels to remain valid until we print the summary
  gLabels.push_back( label );
  record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

// constants defined in outer function
template <typename T, typename Shifter>
void test_constant_arg45(T* first, int count, const std::string label) {
    int i;
  
    start_timer();

    const T x = T(2);
    const T y = T(3);
    const T z = T(12);
    const T k = T(0);

    for(i = 0; i < iterations; ++i) {
        T result = 0;
        for (int n = 0; n < count; ++n) {
            result += Shifter::do_shift( first[n], x, y, z, k );
        }
        check_shifted_sum_arg42<T, Shifter>(result, x, y, z, k, label);
    }

  // need the labels to remain valid until we print the summary
  gLabels.push_back( label );
  record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

// constants passed as template arguments in outer function
template <typename T, typename Shifter, T x, T y, T z, T k>
void test_constant_arg43(T* first, int count, const std::string label) {
    int i;
  
    start_timer();

    for(i = 0; i < iterations; ++i) {
        T result = 0;
        for (int n = 0; n < count; ++n) {
            result += Shifter::do_shift( first[n], x, y, z, k );
        }
        check_shifted_sum_arg42<T, Shifter>(result, x, y, z, k, label);
    }

  // need the labels to remain valid until we print the summary
  gLabels.push_back( label );
  record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

// constants passed as arguments to outer function
template <typename T, typename Shifter>
void test_constant_arg44(T* first, int count, const std::string label, T x, T y, T z, T k) {
    int i;
  
    start_timer();

    for(i = 0; i < iterations; ++i) {
        T result = 0;
        for (int n = 0; n < count; ++n) {
            result += Shifter::do_shift( first[n], x, y, z, k );
        }
        check_shifted_sum_arg42<T, Shifter>(result, x, y, z, k, label);
    }

  // need the labels to remain valid until we print the summary
  gLabels.push_back( label );
  record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

// constants passed as arguments to outer function recursively
template <typename T, typename Shifter>
void test_constant_arg4_resursive(T* first, int count, const std::string &label, T x, T y, T z, T k, int N) {
    int i;
    
    if (N != 0)
        test_constant_arg4_resursive<T,Shifter>(first,count,label,x,y,z,k,N-1);
    else {
  
        start_timer();

        for(i = 0; i < iterations; ++i) {
            T result = 0;
            for (int n = 0; n < count; ++n) {
                result += Shifter::do_shift( first[n], x, y, z, k );
            }
            check_shifted_sum_arg42<T, Shifter>(result, x, y, z, k, label);
        }

        // need the labels to remain valid until we print the summary
        gLabels.push_back( label );
        record_result( timer(), gLabels.back().c_str() );
    }

}

/******************************************************************************/

// recursive, constants are passed in as template parameters
template <int N, typename T, T x, T y, T z, T k >
    struct custom_prop1_template_recursive {
      static T do_shift(T input) { 
        return custom_prop1_template_recursive<N-1,T,x,y,z,k>::do_shift(input);
        }
    };

// this specialization terminates the recursion
template <typename T, T x, T y, T z, T k >
    struct custom_prop1_template_recursive<0,T,x,y,z,k> {
      static T do_shift(T input) { return (z / (x * (y + k))); }
    };

/******************************************************************************/
/******************************************************************************/

template< typename T >
void TestOneType(T x, T y, T z, T k)
{
    const bool isFloat = T(2.1) > T(2);     // darn - can't get this to work at compile time yet
    
    T data[SIZE];
    
    ::fill(data, data+SIZE, T(init_value));
    
    std::string myTypeName( getTypeName<T>() );

    test_constant<T, custom_two<T> >(data,SIZE,myTypeName + " constant verify1");
    test_constant<T, custom_inline1<T> >(data,SIZE,myTypeName + " constant inline");
    test_constant<T, custom_prop1<T> >(data,SIZE,myTypeName + " propagation1");
    test_constant<T, custom_prop2<T> >(data,SIZE,myTypeName + " propagation2");
    test_constant<T, custom_prop3<T> >(data,SIZE,myTypeName + " propagation3");
    test_constant_arg4<T, custom_prop1_arg<T> >(data,SIZE,myTypeName + " propagation1 const arguments inner");
    test_constant_arg42<T, custom_prop1_arg<T> >(data,SIZE,myTypeName + " propagation1 var arguments inner");
    test_constant_arg45<T, custom_prop1_arg<T> >(data,SIZE,myTypeName + " propagation2 var arguments inner");
    test_constant_arg44<T, custom_prop1_arg<T> >(data,SIZE,myTypeName + " propagation1 const arguments outer", 2, 3, 12, 0);
    test_constant_arg44<T, custom_prop1_arg<T> >(data,SIZE,myTypeName + " propagation1 var arguments outer", x, y, z, k);
    test_constant_arg4_global<T, custom_prop1_arg<T> >(data,SIZE,myTypeName + " propagation1 global arguments inner");
    test_constant_arg4_global_const<T, custom_prop1_arg<T> >(data,SIZE,myTypeName + " propagation1 global const arguments inner");
    test_constant_arg4_global_static<T, custom_prop1_arg<T> >(data,SIZE,myTypeName + " propagation1 global static arguments inner");
    test_constant_arg4_static<T, custom_prop1_arg<T> >(data,SIZE,myTypeName + " propagation1 static arguments inner");
    test_constant_arg4_resursive<T, custom_prop1_arg<T> >(data,SIZE,myTypeName + " propagation1 const arguments recursive 10", 2, 3, 12, 0, 10);
    test_constant_arg4_resursive<T, custom_prop1_arg<T> >(data,SIZE,myTypeName + " propagation1 const arguments recursive 50", 2, 3, 12, 0, 50);
    test_constant_arg4_resursive<T, custom_prop1_arg<T> >(data,SIZE,myTypeName + " propagation1 const arguments recursive 100", 2, 3, 12, 0, 100);
    test_constant_arg4_resursive<T, custom_prop1_arg<T> >(data,SIZE,myTypeName + " propagation1 const arguments recursive 500", 2, 3, 12, 0, 500);
    test_constant_arg4_resursive<T, custom_prop1_arg<T> >(data,SIZE,myTypeName + " propagation1 const arguments recursive 1000", 2, 3, 12, 0, 1000);
    
    if (!isFloat) {
        test_constant<T, custom_prop1_template_arg<T, T(2), T(3), T(12), T(0)> >(data,SIZE,myTypeName + " propagation1 template arguments inner");
        test_constant_arg43<T, custom_prop1_arg<T>, T(2), T(3), T(12), T(0) >(data,SIZE,myTypeName + " propagation1 template arguments outer");
        test_constant<T, custom_prop1_template_recursive<10, T, 2, 3, 12, 0> >(data,SIZE,myTypeName + " propagation1 template arguments recursive 10");
        test_constant<T, custom_prop1_template_recursive<50, T, 2, 3, 12, 0> >(data,SIZE,myTypeName + " propagation1 template arguments recursive 50");
        test_constant<T, custom_prop1_template_recursive<100, T, 2, 3, 12, 0> >(data,SIZE,myTypeName + " propagation1 template arguments recursive 100");
        test_constant<T, custom_prop1_template_recursive<500, T, 2, 3, 12, 0> >(data,SIZE,myTypeName + " propagation1 template arguments recursive 500");
        test_constant<T, custom_prop1_template_recursive<1000, T, 2, 3, 12, 0> >(data,SIZE,myTypeName + " propagation1 template arguments recursive 1000");
    }
    
    std::string temp1( myTypeName + " simple constant propagation" );
    summarize( temp1.c_str(), SIZE, iterations, kDontShowGMeans, kDontShowPenalty );

}

/******************************************************************************/

template< typename T >
void TestOneTypeFloat(T x, T y, T z, T k)
{
    //const bool isFloat = T(2.1) > T(2);     // darn - can't get this to work at compile time yet
    
    T data[SIZE];
    
    ::fill(data, data+SIZE, T(init_value));
    
    std::string myTypeName( getTypeName<T>() );

    test_constant<T, custom_two<T> >(data,SIZE,myTypeName + " constant verify1");
    test_constant<T, custom_inline1<T> >(data,SIZE,myTypeName + " constant inline");
    test_constant<T, custom_prop1<T> >(data,SIZE,myTypeName + " propagation1");
    test_constant<T, custom_prop2<T> >(data,SIZE,myTypeName + " propagation2");
    test_constant<T, custom_prop3<T> >(data,SIZE,myTypeName + " propagation3");
    test_constant_arg4<T, custom_prop1_arg<T> >(data,SIZE,myTypeName + " propagation1 const arguments inner");
    test_constant_arg42<T, custom_prop1_arg<T> >(data,SIZE,myTypeName + " propagation1 var arguments inner");
    test_constant_arg45<T, custom_prop1_arg<T> >(data,SIZE,myTypeName + " propagation2 var arguments inner");
    test_constant_arg44<T, custom_prop1_arg<T> >(data,SIZE,myTypeName + " propagation1 const arguments outer", 2, 3, 12, 0);
    test_constant_arg44<T, custom_prop1_arg<T> >(data,SIZE,myTypeName + " propagation1 var arguments outer", x, y, z, k);
    test_constant_arg4_global<T, custom_prop1_arg<T> >(data,SIZE,myTypeName + " propagation1 global arguments inner");
    test_constant_arg4_global_const<T, custom_prop1_arg<T> >(data,SIZE,myTypeName + " propagation1 global const arguments inner");
    test_constant_arg4_global_static<T, custom_prop1_arg<T> >(data,SIZE,myTypeName + " propagation1 global static arguments inner");
    test_constant_arg4_static<T, custom_prop1_arg<T> >(data,SIZE,myTypeName + " propagation1 static arguments inner");
    test_constant_arg4_resursive<T, custom_prop1_arg<T> >(data,SIZE,myTypeName + " propagation1 const arguments recursive 10", 2, 3, 12, 0, 10);
    test_constant_arg4_resursive<T, custom_prop1_arg<T> >(data,SIZE,myTypeName + " propagation1 const arguments recursive 50", 2, 3, 12, 0, 50);
    test_constant_arg4_resursive<T, custom_prop1_arg<T> >(data,SIZE,myTypeName + " propagation1 const arguments recursive 100", 2, 3, 12, 0, 100);
    test_constant_arg4_resursive<T, custom_prop1_arg<T> >(data,SIZE,myTypeName + " propagation1 const arguments recursive 500", 2, 3, 12, 0, 500);
    test_constant_arg4_resursive<T, custom_prop1_arg<T> >(data,SIZE,myTypeName + " propagation1 const arguments recursive 1000", 2, 3, 12, 0, 1000);
    
    std::string temp1( myTypeName + " simple constant propagation" );
    summarize( temp1.c_str(), SIZE, iterations, kDontShowGMeans, kDontShowPenalty );

}

/******************************************************************************/

int main(int argc, char** argv) {
    
    // output command for documentation:
    int i;
    for (i = 0; i < argc; ++i)
        printf("%s ", argv[i] );
    printf("\n");

    if (argc > 1) iterations = atoi(argv[1]);
    if (argc > 2) init_value = (double) atof(argv[2]);

    int32_t x = 2;
    int32_t y = 3;
    int32_t z = 12;
    int32_t k = 0;


    TestOneType<int8_t>(x,y,z,k);
    TestOneType<uint8_t>(x,y,z,k);
    TestOneType<int16_t>(x,y,z,k);
    TestOneType<uint16_t>(x,y,z,k);
    TestOneType<int32_t>(x,y,z,k);
    TestOneType<uint32_t>(x,y,z,k);
    TestOneType<int64_t>(x,y,z,k);
    TestOneType<uint64_t>(x,y,z,k);


    iterations /= 15;
    TestOneTypeFloat<float>(x,y,z,k);
    TestOneTypeFloat<double>(x,y,z,k);
    TestOneTypeFloat<long double>(x,y,z,k);


    return 0;
}

// the end
/******************************************************************************/
/******************************************************************************/
