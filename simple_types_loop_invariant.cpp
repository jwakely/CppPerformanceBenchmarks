/*
    Copyright 2007-2008 Adobe Systems Incorporated
    Copyright 2018-2019 Chris Cox
    Distributed under the MIT License (see accompanying file LICENSE_1_0_0.txt
    or a copy at http://stlab.adobe.com/licenses.html )


Goal:  Test compiler optimizations related to simple language defined types
        and loop invariant code motion.

Assumptions:

    1) The compiler will move loop invariant calculations on simple types out of a loop.
        
        for (i = 0; i < N; ++i)                        temp = A + B + C + D;
            result += input[i] + A+B+C+D;    ==>       for (i = 0; i < N; ++i)
                                                          result += input[i] + temp;

        for (i = 0; i < N; ++i)                        temp = A + B + C + D;
            result += input[i] + A+B+C+D;    ==>       for (i = 0; i < N; ++i)
                                                          result += input[i];
                                                       result += temp * N;

    2) The compiler will move loop invariant memory accesses out of a loop.
        This can also be related to scalar replacement and dead store elimination.
 
        for (i = 0; i < N; ++i)                     result_temp = result[k]
            result[k] += input[i];      ==>         for (i = 0; i < N; ++i)
                                                        result_temp += input[i];
                                                    result[k] = result_temp

        for (i = 0; i < N; ++i)                     result_temp = result.k
            result.k += input[i];       ==>         for (i = 0; i < N; ++i)
                                                        result_temp += input[i];
                                                    result.k = result_temp
        for (i = 0; i < N; ++i)
            result += input[k];         ==>         result += N*input[k];
 
        for (i = 0; i < N; ++i)
            result += struct.g;         ==>         result += N*struct.g;

        for (i = 0; i < N; ++i)
            result[k] = input[i]        ==>         result[k] = input[N-1];

        for (i = 0; i < N; ++i)
            result.k = input[i]         ==>         result.k = input[N-1];

    3) The compiler will move loop invariant function calls out of a loop.
 
        for (i = 0; i < N; ++i)                     temp = cos(K);
            result += cos(K);           ==>         for (i = 0; i < N; ++i)
                                                        result += temp;
        for (i = 0; i < N; ++i)
            result += cos(K);           ==>         result += N * cos(K);

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
#include <type_traits>
#include "benchmark_results.h"
#include "benchmark_timer.h"
#include "benchmark_typenames.h"

/******************************************************************************/

// this constant may need to be adjusted to give reasonable minimum times
// For best results, times should be about 1.0 seconds for the minimum test run
int iterations = 800000;


// 8000 items, or between 8k and 64k of data
// this is intended to remain within the L2 cache of most common CPUs
const int SIZE = 8000;


// initial value for filling our arrays, may be changed from the command line
double init_value = 1.0;

/******************************************************************************/

#include "benchmark_shared_tests.h"

static std::deque<std::string> gLabels;

/******************************************************************************/

// v1 is constant in the function, so we can move the addition or subtraction of it outside the loop entirely,
// converting it to a multiply and a summation of the input array.
// Note that this is always legal for integers.
// It can only be applied to floating point if using inexact math (relaxed IEEE rules).
template <typename T, typename Shifter>
void test_hoisted_variable1(T* first, int count, T v1, const std::string &label) {
  int i;
  
  start_timer();
  
  for(i = 0; i < iterations; ++i) {
    T result = 0;
    for (int n = 0; n < count; ++n) {
        result += first[n];
    }
    result += count * v1;
    check_shifted_variable_sum<T, Shifter>(result, v1);
  }

  // need the labels to remain valid until we print the summary
  gLabels.push_back( label );
  record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

template <typename T, typename Shifter>
void test_variable1(T* first, int count, T v1, const std::string &label) {
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
void test_variable4(T* first, int count, T v1, T v2, T v3, T v4, const std::string &label) {
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

template <typename T>
void test_array_const_out_opt(T* first, T *second, int count, int v1, const std::string &label) {
  
  start_timer();
  
  for(int i = 0; i < iterations; ++i) {
    T temp = (T)SIZE * T(init_value);
    T second_temp = 0;
    for (int n = 0; n < count; ++n) {
        second_temp += first[n];
    }
    second[v1] = second_temp;
    if (!tolerance_equal<T>(second[v1],temp))
        printf("test %s failed\n", label.c_str() );
  }

  // need the labels to remain valid until we print the summary
  gLabels.push_back( label );
  record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

template <typename T>
void test_array_const_out(T* first, T *second, int count, int v1, const std::string &label) {
  
  start_timer();
  
  for(int i = 0; i < iterations; ++i) {
    T temp = (T)SIZE * T(init_value);
    second[v1] = 0;
    for (int n = 0; n < count; ++n) {
        second[v1] += first[n];
    }
    if (!tolerance_equal<T>(second[v1],temp))
        printf("test %s failed\n", label.c_str() );
  }

  // need the labels to remain valid until we print the summary
  gLabels.push_back( label );
  record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

template <typename T, typename SS>
void test_struct_const_out_opt(T* first, SS *second, int count, const std::string &label) {
  
  start_timer();
  
  for(int i = 0; i < iterations; ++i) {
    T temp = (T)SIZE * T(init_value);
    T second_temp = 0;
    for (int n = 0; n < count; ++n) {
        second_temp += first[n];
    }
    second->g = second_temp;
    if (!tolerance_equal<T>(second->g,temp))
        printf("test %s failed\n", label.c_str() );
  }

  // need the labels to remain valid until we print the summary
  gLabels.push_back( label );
  record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

template <typename T, typename SS>
void test_struct_const_out(T* first, SS *second, int count, const std::string &label) {
  
  start_timer();
  
  for(int i = 0; i < iterations; ++i) {
    T temp = (T)SIZE * T(init_value);
    second->g = 0;
    for (int n = 0; n < count; ++n) {
        second->g += first[n];
    }
    if (!tolerance_equal<T>(second->g,temp))
        printf("test %s failed\n", label.c_str() );
  }

  // need the labels to remain valid until we print the summary
  gLabels.push_back( label );
  record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/
/******************************************************************************/

// Note that this is always legal for integers.
// It can only be applied to floating point if using inexact math (relaxed IEEE rules).
template <typename T>
void test_array_const_in_opt(T* first, int count, int v1, const std::string &label) {
  
  start_timer();
  
  for(int i = 0; i < iterations; ++i) {
    T temp = (T)SIZE * T(init_value);
    T result = count * first[v1];
    if (!tolerance_equal<T>(result,temp))
        printf("test %s failed\n", label.c_str() );
  }

  // need the labels to remain valid until we print the summary
  gLabels.push_back( label );
  record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

template <typename T>
void test_array_const_in(T* first, int count, int v1, const std::string &label) {
  
  start_timer();
  
  for(int i = 0; i < iterations; ++i) {
    T temp = (T)SIZE * T(init_value);
    T result = 0;
    for (int n = 0; n < count; ++n) {
        result += first[v1];
    }
    if (!tolerance_equal<T>(result,temp))
        printf("test %s failed\n", label.c_str() );
  }

  // need the labels to remain valid until we print the summary
  gLabels.push_back( label );
  record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

// Note that this is always legal for integers.
// It can only be applied to floating point if using inexact math (relaxed IEEE rules).
template <typename T, typename SS>
void test_struct_const_in_opt(SS *second, int count, const std::string &label) {
  
  start_timer();
  
  for(int i = 0; i < iterations; ++i) {
    T temp = (T)SIZE * T(init_value);
    T result = count * second->g;
    if (!tolerance_equal<T>(result,temp))
        printf("test %s failed\n", label.c_str() );
  }

  // need the labels to remain valid until we print the summary
  gLabels.push_back( label );
  record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

template <typename T, typename SS>
void test_struct_const_in(SS *second, int count, const std::string &label) {
  
  start_timer();
  
  for(int i = 0; i < iterations; ++i) {
    T temp = (T)SIZE * T(init_value);
    T result = 0;
    for (int n = 0; n < count; ++n) {
        result += second->g;
    }
    if (!tolerance_equal<T>(result,temp))
        printf("test %s failed\n", label.c_str() );
  }

  // need the labels to remain valid until we print the summary
  gLabels.push_back( label );
  record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/
/******************************************************************************/

template <typename T>
void test_replace_array_opt(T* first, T *second, int count, int v1, const std::string &label) {
  
  start_timer();
  
  for(int i = 0; i < iterations; ++i) {
    second[v1] = first[count-1];
    if (!tolerance_equal<T>(second[v1],first[count-1]))
        printf("test %s failed\n", label.c_str() );
  }

  // need the labels to remain valid until we print the summary
  gLabels.push_back( label );
  record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

template <typename T>
void test_replace_array(T* first, T *second, int count, int v1, const std::string &label) {
  
  start_timer();
  
  for(int i = 0; i < iterations; ++i) {
    for (int n = 0; n < count; ++n) {
        second[v1] = first[n];
    }
    if (!tolerance_equal<T>(second[v1],first[count-1]))
        printf("test %s failed\n", label.c_str() );
  }

  // need the labels to remain valid until we print the summary
  gLabels.push_back( label );
  record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

template <typename T, typename SS>
void test_replace_struct_opt(T* first, SS *second, int count, const std::string &label) {
  
  start_timer();
  
  for(int i = 0; i < iterations; ++i) {
    second->g = first[count-1];
    if (!tolerance_equal<T>(second->g,first[count-1]))
        printf("test %s failed\n", label.c_str() );
  }

  // need the labels to remain valid until we print the summary
  gLabels.push_back( label );
  record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

template <typename T, typename SS>
void test_replace_struct(T* first, SS *second, int count, const std::string &label) {
  
  start_timer();
  
  for(int i = 0; i < iterations; ++i) {
    for (int n = 0; n < count; ++n) {
        second->g = first[n];
    }
    if (!tolerance_equal<T>(second->g,first[count-1]))
        printf("test %s failed\n", label.c_str() );
  }

  // need the labels to remain valid until we print the summary
  gLabels.push_back( label );
  record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/
/******************************************************************************/

// cannot use constexpr or consteval yet
template<typename T>
T test_power( const T in ) {
    return pow( in, 2.2573 );
}

template<typename T>
T test_cos( const T in ) {
    return cos( in + 0.2573 );
}

template<typename T>
T test_sqr( const T in ) {
    return in * in;
}

// good album, but a poor hash function
template<typename T>
T test_hash( const T in ) {
    const T a(static_cast<T>(90125)), b(static_cast<T>(123)), c(static_cast<T>(98765.4321));
    return T( T(in * a + b) * a + b) / c;
}

/******************************************************************************/

template <typename T, typename Func>
void test_const_function_opt(T* first, int count, int v1, Func opop, const std::string &label) {
    
    start_timer();
    
    for(int i = 0; i < iterations; ++i) {
        T expected = (T)SIZE * T(opop(v1));
        T result = SIZE * T(opop(v1));
        if (!tolerance_equal<T>(result,expected))
            printf("test %s failed\n", label.c_str() );
    }
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

template <typename T, typename Func>
void test_const_function_halfopt(T* first, int count, int v1, Func opop, const std::string &label) {
    
    start_timer();
    
    T value = opop(v1);
    T expected = (T)SIZE * T(opop(v1));
    for(int i = 0; i < iterations; ++i) {
        T result = 0;
        for (int n = 0; n < count; ++n) {
            result += value;
        }
        if (!tolerance_equal<T>(result,expected))
            printf("test %s failed\n", label.c_str() );
    }
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

template <typename T, typename Func>
void test_const_function(T* first, int count, int v1, Func opop, const std::string &label) {
    
    start_timer();
    
    for(int i = 0; i < iterations; ++i) {
        T expected = (T)SIZE * T(opop(v1));
        T result = 0;
        for (int n = 0; n < count; ++n) {
            result += opop(v1);
        }
        if (!tolerance_equal<T>(result,expected))
            printf("test %s failed\n", label.c_str() );
    }
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/
/******************************************************************************/

template< typename T >
typename std::enable_if<std::is_floating_point<T>::value,void>::type
 TestLoopsIntegerOnly(T *data, T var1int1, T var1int2, T var1int3, T var1int4 )
{
    // can't do bit operations on floating point values
}

template< typename T >
typename std::enable_if<std::is_integral<T>::value,void>::type
 TestLoopsIntegerOnly(T *data, T var1int1, T var1int2, T var1int3, T var1int4 )
{
    std::string myTypeName( getTypeName<T>() );
    
    test_variable1< T, custom_variable_and<T> > (data, SIZE, var1int1, myTypeName + " variable and");
    test_variable4< T, custom_multiple_variable_and<T> > (data, SIZE, var1int1, var1int2, var1int3, var1int4,
        myTypeName + " multiple variable and");
    test_variable4< T, custom_multiple_variable_and2<T> > (data, SIZE, var1int1, var1int2, var1int3, var1int4,
        myTypeName + " multiple variable and2");

    test_variable1< T, custom_variable_or<T> > (data, SIZE, var1int1, myTypeName + " variable or");
    test_variable4< T, custom_multiple_variable_or<T> > (data, SIZE, var1int1, var1int2, var1int3, var1int4,
        myTypeName + " multiple variable or");
    test_variable4< T, custom_multiple_variable_or2<T> > (data, SIZE, var1int1, var1int2, var1int3, var1int4,
        myTypeName + " multiple variable or2");

    test_variable1< T, custom_variable_xor<T> > (data, SIZE, var1int1, myTypeName + " variable xor");
    test_variable4< T, custom_multiple_variable_xor<T> > (data, SIZE, var1int1, var1int2, var1int3, var1int4,
        myTypeName + " multiple variable xor");
    test_variable4< T, custom_multiple_variable_xor2<T> > (data, SIZE, var1int1, var1int2, var1int3, var1int4,
        myTypeName + " multiple variable xor2");
}

/******************************************************************************/

template<typename T>
struct simple_struct {
    T a,b,c,d,e,f,g,h,i,j,k,l,m;
};

template< typename T >
void TestLoops(double temp)
{
    T data[ SIZE ];
    
    std::string myTypeName( getTypeName<T>() );
    
    gLabels.clear();
    
    ::fill(data, data+SIZE, T(init_value));
    T var1int1, var1int2, var1int3, var1int4;
    var1int1 = T(temp);
    var1int2 = var1int1 * T(2);
    var1int3 = var1int1 + T(2);
    var1int4 = var1int1 + var1int2 / var1int3;
    

    test_variable1< T, custom_add_variable<T> > (data, SIZE, var1int1, myTypeName + " variable add");
    test_hoisted_variable1< T, custom_add_variable<T> > (data, SIZE, var1int1, myTypeName + " variable add hoisted");
    test_variable4< T, custom_add_multiple_variable<T> > (data, SIZE, var1int1, var1int2, var1int3, var1int4,
        myTypeName + " multiple variable adds");
    test_variable4< T, custom_add_multiple_variable2<T> > (data, SIZE, var1int1, var1int2, var1int3, var1int4,
        myTypeName + " multiple variable adds2");

    test_variable1< T, custom_sub_variable<T> > (data, SIZE, var1int1, myTypeName + " variable subtract");
    test_variable4< T, custom_sub_multiple_variable<T> > (data, SIZE, var1int1, var1int2, var1int3, var1int4,
        myTypeName + " multiple variable subtracts");
    test_variable4< T, custom_sub_multiple_variable2<T> > (data, SIZE, var1int1, var1int2, var1int3, var1int4,
        myTypeName + " multiple variable subtracts2");
    
    test_variable1< T, custom_multiply_variable<T> > (data, SIZE, var1int1, myTypeName + " variable multiply");
    test_variable4< T, custom_multiply_multiple_variable<T> > (data, SIZE, var1int1, var1int2, var1int3, var1int4,
        myTypeName + " multiple variable multiplies");
    test_variable4< T, custom_multiply_multiple_variable2<T> > (data, SIZE, var1int1, var1int2, var1int3, var1int4,
        myTypeName + " multiple variable multiplies2");
    test_variable4< T, custom_multiply_multiple_variable3<T> > (data, SIZE, var1int1, var1int2, var1int3, var1int4,
        myTypeName + " multiple variable multiplies3");

    test_variable1< T, custom_divide_variable<T> > (data, SIZE, var1int1, myTypeName + " variable divide");
    test_variable4< T, custom_divide_multiple_variable<T> > (data, SIZE, var1int1, var1int2, var1int3, var1int4,
        myTypeName + " multiple variable divides");
    test_variable4< T, custom_divide_multiple_variable2<T> > (data, SIZE, var1int1, var1int2, var1int3, var1int4,
        myTypeName + " multiple variable divides2");
    
    test_variable4< T, custom_mixed_multiple_variable<T> > (data, SIZE, var1int1, var1int2, var1int3, var1int4,
        myTypeName + " multiple variable mixed");
    test_variable4< T, custom_mixed_multiple_variable2<T> > (data, SIZE, var1int1, var1int2, var1int3, var1int4,
        myTypeName + " multiple variable mixed2");
    
    TestLoopsIntegerOnly<T>(data, var1int1, var1int2, var1int3, var1int4);
    
    std::string temp1( myTypeName + " loop invariant" );
    summarize( temp1.c_str(), SIZE, iterations, kDontShowGMeans, kDontShowPenalty );

    
    ::fill(data, data+SIZE, T(init_value));
    
    const int second_data_limit = 50;   // must be less than SIZE
    T data2[second_data_limit];
    int index( init_value + 1234567 );
    index %= second_data_limit;
    
    simple_struct<T> dataStruct;
    dataStruct.g = T(init_value);
    dataStruct.a = T(init_value);
    dataStruct.b = T(init_value);
    dataStruct.h = T(init_value);
    dataStruct.l = T(init_value);
    dataStruct.m = T(init_value);


    test_array_const_out_opt<T> (data, data2, SIZE, index, myTypeName + " sum array const_out opt");
    test_array_const_out<T> (data, data2, SIZE, index, myTypeName + " sum array const_out");
    
    test_struct_const_out_opt<T> (data, &dataStruct, SIZE, myTypeName + " sum struct const_out opt");
    test_struct_const_out<T> (data, &dataStruct, SIZE, myTypeName + " sum struct const_out");

    test_array_const_in_opt<T> (data, SIZE, index, myTypeName + " sum array const_in opt");
    test_array_const_in<T> (data, SIZE, index, myTypeName + " sum array const_in");
    
    dataStruct.g = T(init_value);
    dataStruct.a = T(init_value);
    dataStruct.m = T(init_value);
    test_struct_const_in_opt<T> ( &dataStruct, SIZE, myTypeName + " sum struct const_in opt");
    test_struct_const_in<T> ( &dataStruct, SIZE, myTypeName + " sum struct const_in");

    test_replace_array_opt<T> (data, data2, SIZE, index, myTypeName + " replace array opt");
    test_replace_array<T> (data, data2, SIZE, index, myTypeName + " replace array");
    
    test_replace_struct_opt<T> (data, &dataStruct, SIZE, myTypeName + " replace struct opt");
    test_replace_struct<T> (data, &dataStruct, SIZE, myTypeName + " replace struct");
    
    std::string temp2( myTypeName + " loop memory invariant" );
    summarize( temp2.c_str(), SIZE, iterations, kDontShowGMeans, kDontShowPenalty );


    T value1( init_value + 2 );
    
    test_const_function_opt( data, SIZE, value1, test_power<T>, myTypeName + " const function power opt" );
    test_const_function_halfopt( data, SIZE, value1, test_power<T>, myTypeName + " const function power halfopt" );
    test_const_function( data, SIZE, value1, test_power<T>, myTypeName + " const function power" );
    test_const_function_opt( data, SIZE, value1, test_cos<T>, myTypeName + " const function cosine opt" );
    test_const_function_halfopt( data, SIZE, value1, test_cos<T>, myTypeName + " const function cosine halffopt" );
    test_const_function( data, SIZE, value1, test_cos<T>, myTypeName + " const function cosine" );
    test_const_function_opt( data, SIZE, value1, test_sqr<T>, myTypeName + " const function square opt" );
    test_const_function_halfopt( data, SIZE, value1, test_sqr<T>, myTypeName + " const function square halfopt" );
    test_const_function( data, SIZE, value1, test_sqr<T>, myTypeName + " const function square" );
    test_const_function_opt( data, SIZE, value1, test_hash<T>, myTypeName + " const function hash opt" );
    test_const_function_halfopt( data, SIZE, value1, test_hash<T>, myTypeName + " const function hash halfopt" );
    test_const_function( data, SIZE, value1, test_hash<T>, myTypeName + " const function hash" );
    
    std::string temp3( myTypeName + " loop invariant functions" );
    summarize( temp3.c_str(), SIZE, iterations, kDontShowGMeans, kDontShowPenalty );
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
    
    TestLoops<int8_t>(temp);
    TestLoops<uint8_t>(temp);
    TestLoops<int16_t>(temp);
    TestLoops<uint16_t>(temp);
    TestLoops<int32_t>(temp);
    TestLoops<uint32_t>(temp);
    
    iterations /= 4;
    TestLoops<int64_t>(temp);
    TestLoops<uint64_t>(temp);
    TestLoops<float>(temp);
    TestLoops<double>(temp);
    TestLoops<long double>(temp);
    
    return 0;
}

// the end
/******************************************************************************/
/******************************************************************************/
