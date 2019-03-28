/*
    Copyright 2008 Adobe Systems Incorporated
    Copyright 2018-2019 Chris Cox
    Distributed under the MIT License (see accompanying file LICENSE_1_0_0.txt
    or a copy at http://stlab.adobe.com/licenses.html )


Goal:  Test performance of various idioms for calculating the inner product of two sequences.

NOTE:  Inner products are common in mathematical and geometry processing applications,
        plus some audio and image processing.


Assumptions:
    1) The compiler will optimize inner product operations.

    2) The compiler may recognize ineffecient inner product idioms
        and substitute efficient methods when it can.
        NOTE: the best method is highly dependent on the data types and CPU architecture

    3) std::inner_product will be well optimized for all types and containers.


*/

/******************************************************************************/

#include "benchmark_stdint.hpp"
#include <cstddef>
#include <cstdio>
#include <ctime>
#include <cmath>
#include <cstdlib>
#include <numeric>
#include <algorithm>
#include <deque>
#include <string>
#include "benchmark_results.h"
#include "benchmark_timer.h"
#include "benchmark_algorithms.h"
#include "benchmark_typenames.h"

/******************************************************************************/
/******************************************************************************/

// this constant may need to be adjusted to give reasonable minimum times
// For best results, times should be about 1.0 seconds for the minimum test run
int iterations = 1600000;


// 8000 items, or between 8 and 64k of data
// this is intended to remain within the L2 cache of most common CPUs
const int SIZE = 8000;


// initial value for filling our arrays, may be changed from the command line
int32_t init_value = 3;

/******************************************************************************/
/******************************************************************************/

// LAME - but C++ doesn't have a truely universal abs() function yet
uint8_t  abs(  uint8_t val ) { return val; }
uint16_t abs( uint16_t val ) { return val; }
uint32_t abs( uint32_t val ) { return val; }
uint64_t abs( uint64_t val ) { return val; }

template<typename T>
T absv(T val) {
    return abs(val);    // use math.h version for floats, stdlib for signed ints, and above for unsigned ints
}

template<typename T>
inline void check_sum(T result, const std::string &label) {
    T target = T(init_value)*T(init_value)*SIZE;
    if ( absv( result - target ) > T(1.0e-6) )
        printf("test %s failed\n", label.c_str());
}

/******************************************************************************/
/******************************************************************************/

template<typename Iter, typename T>
T inner_product_std(Iter first, Iter second, const size_t count)
{
    T sum( 0 );
    sum = std::inner_product( first, first+count, second, sum );
    return sum;
}

/******************************************************************************/

// a trivial for loop
template<typename Iter, typename T>
T inner_product1(Iter first, Iter second, const size_t count)
{
    T sum( 0 );
    for (size_t j = 0; j < count; ++j) {
        sum += first[j] * second[j];
    }
    return sum;
}

/******************************************************************************/

// a trivial iterator style loop
template<typename Iter, typename T>
T inner_product2(Iter first, Iter second, const size_t count)
{
    T sum( 0 );
    auto end = first + count;
    while (first != end) {
        sum += *first * *second;
        first++;
        second++;
    }
    return sum;
}

/******************************************************************************/

// unroll 2X
template<typename Iter, typename T>
T inner_product3(Iter first, Iter second, const size_t count)
{
    size_t j;
    T sum( 0 );
    for (j = 0; j < (count - 1); j += 2) {
        T value0 = first[j+0] * second[j+0];
        T value1 = first[j+1] * second[j+1];
        sum += value0;
        sum += value1;
    }
    for (; j < count; ++j) {
        sum += first[j] * second[j];
    }
    return sum;
}

/******************************************************************************/

// unroll 4X
template<typename Iter, typename T>
T inner_product4(Iter first, Iter second, const size_t count)
{
    size_t j;
    T sum( 0 );
    for (j = 0; j < (count - 3); j += 4) {
        T value0 = first[j+0] * second[j+0];
        T value1 = first[j+1] * second[j+1];
        T value2 = first[j+2] * second[j+2];
        T value3 = first[j+3] * second[j+3];
        sum += value0;
        sum += value1;
        sum += value2;
        sum += value3;
    }
    for (; j < count; ++j) {
        sum += first[j] * second[j];
    }
    return sum;
}

/******************************************************************************/

// unroll 8X
template<typename Iter, typename T>
T inner_product5(Iter first, Iter second, const size_t count)
{
    size_t j;
    T sum( 0 );
    for (j = 0; j < (count - 7); j += 8) {
        T value0 = first[j+0] * second[j+0];
        T value1 = first[j+1] * second[j+1];
        T value2 = first[j+2] * second[j+2];
        T value3 = first[j+3] * second[j+3];
        T value4 = first[j+4] * second[j+4];
        T value5 = first[j+5] * second[j+5];
        T value6 = first[j+6] * second[j+6];
        T value7 = first[j+7] * second[j+7];
        sum += value0;
        sum += value1;
        sum += value2;
        sum += value3;
        sum += value4;
        sum += value5;
        sum += value6;
        sum += value7;
    }
    for (; j < count; ++j) {
        sum += first[j] * second[j];
    }
    return sum;
}

/******************************************************************************/

// unroll 16X
template<typename Iter, typename T>
T inner_product6(Iter first, Iter second, const size_t count)
{
    size_t j;
    T sum( 0 );
    for (j = 0; j < (count - 15); j += 16) {
        T value0 = first[j+0] * second[j+0];
        T value1 = first[j+1] * second[j+1];
        T value2 = first[j+2] * second[j+2];
        T value3 = first[j+3] * second[j+3];
        T value4 = first[j+4] * second[j+4];
        T value5 = first[j+5] * second[j+5];
        T value6 = first[j+6] * second[j+6];
        T value7 = first[j+7] * second[j+7];
        T value8 = first[j+0] * second[j+8];
        T value9 = first[j+1] * second[j+9];
        T valueA = first[j+2] * second[j+10];
        T valueB = first[j+3] * second[j+11];
        T valueC = first[j+4] * second[j+12];
        T valueD = first[j+5] * second[j+13];
        T valueE = first[j+6] * second[j+14];
        T valueF = first[j+7] * second[j+15];
        sum += value0;
        sum += value1;
        sum += value2;
        sum += value3;
        sum += value4;
        sum += value5;
        sum += value6;
        sum += value7;
        sum += value8;
        sum += value9;
        sum += valueA;
        sum += valueB;
        sum += valueC;
        sum += valueD;
        sum += valueE;
        sum += valueF;
    }
    for (; j < count; ++j) {
        sum += first[j] * second[j];
    }
    return sum;
}

/******************************************************************************/

// unroll 2X with two accumulator variables
template<typename Iter, typename T>
T inner_product7(Iter first, Iter second, const size_t count)
{
    size_t j;
    T sum( 0 );
    T sum1( 0 );
    for (j = 0; j < (count - 1); j += 2) {
        T value0 = first[j+0] * second[j+0];
        T value1 = first[j+1] * second[j+1];
        sum += value0;
        sum1 += value1;
    }
    for (; j < count; ++j) {
        sum += first[j] * second[j];
    }
    sum += sum1;
    return sum;
}

/******************************************************************************/

// unroll 4X with four accumulator variables
template<typename Iter, typename T>
T inner_product8(Iter first, Iter second, const size_t count)
{
    size_t j;
    T sum( 0 );
    T sum1( 0 );
    T sum2( 0 );
    T sum3( 0 );
    for (j = 0; j < (count - 3); j += 4) {
        T value0 = first[j+0] * second[j+0];
        T value1 = first[j+1] * second[j+1];
        T value2 = first[j+2] * second[j+2];
        T value3 = first[j+3] * second[j+3];
        sum += value0;
        sum1 += value1;
        sum2 += value2;
        sum3 += value3;
    }
    for (; j < count; ++j) {
        sum += first[j] * second[j];
    }
    sum += sum1 + sum2 + sum3;
    return sum;
}

/******************************************************************************/

// unroll 8X with four accumulator variables
template<typename Iter, typename T>
T inner_product9(Iter first, Iter second, const size_t count)
{
    size_t j;
    T sum( 0 );
    T sum1( 0 );
    T sum2( 0 );
    T sum3( 0 );
    for (j = 0; j < (count - 7); j += 8) {
        T value0 = first[j+0] * second[j+0];
        T value1 = first[j+1] * second[j+1];
        T value2 = first[j+2] * second[j+2];
        T value3 = first[j+3] * second[j+3];
        T value4 = first[j+4] * second[j+4];
        T value5 = first[j+5] * second[j+5];
        T value6 = first[j+6] * second[j+6];
        T value7 = first[j+7] * second[j+7];
        sum += value0;
        sum1 += value1;
        sum2 += value2;
        sum3 += value3;
        sum += value4;
        sum1 += value5;
        sum2 += value6;
        sum3 += value7;
    }
    for (; j < count; ++j) {
        sum += first[j] * second[j];
    }
    sum += sum1 + sum2 + sum3;
    return sum;
}

/******************************************************************************/

// unroll 16X with four accumulator variables
template<typename Iter, typename T>
T inner_product10(Iter first, Iter second, const size_t count)
{
    size_t j;
    T sum( 0 );
    T sum1( 0 );
    T sum2( 0 );
    T sum3( 0 );
    for (j = 0; j < (count - 15); j += 16) {
        T value0 = first[j+0] * second[j+0];
        T value1 = first[j+1] * second[j+1];
        T value2 = first[j+2] * second[j+2];
        T value3 = first[j+3] * second[j+3];
        T value4 = first[j+4] * second[j+4];
        T value5 = first[j+5] * second[j+5];
        T value6 = first[j+6] * second[j+6];
        T value7 = first[j+7] * second[j+7];
        T value8 = first[j+0] * second[j+8];
        T value9 = first[j+1] * second[j+9];
        T valueA = first[j+2] * second[j+10];
        T valueB = first[j+3] * second[j+11];
        T valueC = first[j+4] * second[j+12];
        T valueD = first[j+5] * second[j+13];
        T valueE = first[j+6] * second[j+14];
        T valueF = first[j+7] * second[j+15];
        sum += value0;
        sum1 += value1;
        sum2 += value2;
        sum3 += value3;
        sum += value4;
        sum1 += value5;
        sum2 += value6;
        sum3 += value7;
        sum += value8;
        sum1 += value9;
        sum2 += valueA;
        sum3 += valueB;
        sum += valueC;
        sum1 += valueD;
        sum2 += valueE;
        sum3 += valueF;
    }
    for (; j < count; ++j) {
        sum += first[j] * second[j];
    }
    sum += sum1 + sum2 + sum3;
    return sum;
}

/******************************************************************************/
/******************************************************************************/

std::deque<std::string> gLabels;

template <typename T, typename Summer>
void test_inner_product( const T* first, const T* second, const size_t count, Summer func, const std::string label) {

    start_timer();

    for(int i = 0; i < iterations; ++i) {
        T sum = func( first, second, count );
        check_sum( sum, label );
    }
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/
/******************************************************************************/

// NOTE - can't make generic template template argument without C++17
// I would like to have TestOneFunction to handle all the types and if's, but need to use different types with it inside
// see sum_sequence.cpp

template<typename T>
void TestOneType()
{
    std::string myTypeName( getTypeName<T>() );
    
    const bool isFloat = benchmark::isFloat<T>();
    const bool isSigned = benchmark::isSigned<T>();
    
    gLabels.clear();
    
    T data[SIZE];
    T dataB[SIZE];

    fill(data, data+SIZE, T(init_value));
    fill(dataB, dataB+SIZE, T(init_value));


    test_inner_product( data, dataB, SIZE, inner_product_std<const T*,T>, myTypeName + " std::inner_product to " + myTypeName );
    if (isFloat) {
        if (sizeof(T) < sizeof(double)) {
            test_inner_product( data, dataB, SIZE, inner_product_std<const T*, double>, myTypeName + " std::inner_product to double" );
        }
    } else {
        if (isSigned) {
            if (sizeof(T) < sizeof(int16_t))
                test_inner_product( data, dataB, SIZE, inner_product_std<const T*, int16_t>, myTypeName + " std::inner_product to int16_t" );
            if (sizeof(T) < sizeof(int32_t))
                test_inner_product( data, dataB, SIZE, inner_product_std<const T*, int32_t>, myTypeName + " std::inner_product to int32_t" );
            if (sizeof(T) < sizeof(int64_t))
                test_inner_product( data, dataB, SIZE, inner_product_std<const T*, int64_t>, myTypeName + " std::inner_product to int64_t" );
        } else {
            if (sizeof(T) < sizeof(uint16_t))
                test_inner_product( data, dataB, SIZE, inner_product_std<const T*, uint16_t>, myTypeName + " std::inner_product to uint16_t" );
            if (sizeof(T) < sizeof(uint32_t))
                test_inner_product( data, dataB, SIZE, inner_product_std<const T*, uint32_t>, myTypeName + " std::inner_product to uint32_t" );
            if (sizeof(T) < sizeof(uint64_t))
                test_inner_product( data, dataB, SIZE, inner_product_std<const T*, uint64_t>, myTypeName + " std::inner_product to uint64_t" );
        }
    }
    
    test_inner_product( data, dataB, SIZE, inner_product1<const T*,T>, myTypeName + " inner_product1 to " + myTypeName );
    if (isFloat) {
        if (sizeof(T) < sizeof(double)) {
            test_inner_product( data, dataB, SIZE, inner_product1<const T*, double>, myTypeName + " inner_product1 to double" );
        }
    } else {
        if (isSigned) {
            if (sizeof(T) < sizeof(int16_t))
                test_inner_product( data, dataB, SIZE, inner_product1<const T*, int16_t>, myTypeName + " inner_product1 to int16_t" );
            if (sizeof(T) < sizeof(int32_t))
                test_inner_product( data, dataB, SIZE, inner_product1<const T*, int32_t>, myTypeName + " inner_product1 to int32_t" );
            if (sizeof(T) < sizeof(int64_t))
                test_inner_product( data, dataB, SIZE, inner_product1<const T*, int64_t>, myTypeName + " inner_product1 to int64_t" );
        } else {
            if (sizeof(T) < sizeof(uint16_t))
                test_inner_product( data, dataB, SIZE, inner_product1<const T*, uint16_t>, myTypeName + " inner_product1 to uint16_t" );
            if (sizeof(T) < sizeof(uint32_t))
                test_inner_product( data, dataB, SIZE, inner_product1<const T*, uint32_t>, myTypeName + " inner_product1 to uint32_t" );
            if (sizeof(T) < sizeof(uint64_t))
                test_inner_product( data, dataB, SIZE, inner_product1<const T*, uint64_t>, myTypeName + " inner_product1 to uint64_t" );
        }
    }
    
    test_inner_product( data, dataB, SIZE, inner_product2<const T*,T>, myTypeName + " inner_product2 to " + myTypeName );
    if (isFloat) {
        if (sizeof(T) < sizeof(double)) {
            test_inner_product( data, dataB, SIZE, inner_product2<const T*, double>, myTypeName + " inner_product2 to double" );
        }
    } else {
        if (isSigned) {
            if (sizeof(T) < sizeof(int16_t))
                test_inner_product( data, dataB, SIZE, inner_product2<const T*, int16_t>, myTypeName + " inner_product2 to int16_t" );
            if (sizeof(T) < sizeof(int32_t))
                test_inner_product( data, dataB, SIZE, inner_product2<const T*, int32_t>, myTypeName + " inner_product2 to int32_t" );
            if (sizeof(T) < sizeof(int64_t))
                test_inner_product( data, dataB, SIZE, inner_product2<const T*, int64_t>, myTypeName + " inner_product2 to int64_t" );
        } else {
            if (sizeof(T) < sizeof(uint16_t))
                test_inner_product( data, dataB, SIZE, inner_product2<const T*, uint16_t>, myTypeName + " inner_product2 to uint16_t" );
            if (sizeof(T) < sizeof(uint32_t))
                test_inner_product( data, dataB, SIZE, inner_product2<const T*, uint32_t>, myTypeName + " inner_product2 to uint32_t" );
            if (sizeof(T) < sizeof(uint64_t))
                test_inner_product( data, dataB, SIZE, inner_product2<const T*, uint64_t>, myTypeName + " inner_product2 to uint64_t" );
        }
    }
    
    test_inner_product( data, dataB, SIZE, inner_product3<const T*,T>, myTypeName + " inner_product3 to " + myTypeName );
    test_inner_product( data, dataB, SIZE, inner_product4<const T*,T>, myTypeName + " inner_product4 to " + myTypeName );
    test_inner_product( data, dataB, SIZE, inner_product5<const T*,T>, myTypeName + " inner_product5 to " + myTypeName );
    test_inner_product( data, dataB, SIZE, inner_product6<const T*,T>, myTypeName + " inner_product6 to " + myTypeName );
    test_inner_product( data, dataB, SIZE, inner_product7<const T*,T>, myTypeName + " inner_product7 to " + myTypeName );
    test_inner_product( data, dataB, SIZE, inner_product8<const T*,T>, myTypeName + " inner_product8 to " + myTypeName );
    test_inner_product( data, dataB, SIZE, inner_product9<const T*,T>, myTypeName + " inner_product9 to " + myTypeName );
    test_inner_product( data, dataB, SIZE, inner_product10<const T*,T>, myTypeName + " inner_product10 to " + myTypeName );
    
    
    std::string temp1( myTypeName + " inner_product" );
    summarize(temp1.c_str(), SIZE, iterations, kDontShowGMeans, kDontShowPenalty );

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


    TestOneType<int8_t>();
    TestOneType<uint8_t>();
    TestOneType<int16_t>();
    TestOneType<uint16_t>();
    TestOneType<int32_t>();
    TestOneType<uint32_t>();
    
    iterations /= 4;
    TestOneType<int64_t>();
    TestOneType<uint64_t>();
    TestOneType<float>();
    TestOneType<double>();
    TestOneType<long double>();


    return 0;
}

// the end
/******************************************************************************/
/******************************************************************************/
