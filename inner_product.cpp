/*
    Copyright 2008 Adobe Systems Incorporated
    Copyright 2018 Chris Cox
    Distributed under the MIT License (see accompanying file LICENSE_1_0_0.txt
    or a copy at http://stlab.adobe.com/licenses.html )


Goal:  test performance of various idioms for calculating the inner product of two sequences

NOTE:    Inner products are common in mathematical and geometry processing applications


Assumptions:
    1) The compiler will optimize inner product operations
    
    2) The compiler may recognize ineffecient inner product idioms
        and substitute efficient methods when it can.
        NOTE: the best method is highly dependent on the data types and CPU architecture



*/

/******************************************************************************/

#include "benchmark_stdint.hpp"
#include <cstddef>
#include <cstdio>
#include <ctime>
#include <cmath>
#include <cstdlib>
#include <numeric>
#include "benchmark_results.h"
#include "benchmark_timer.h"
#include "benchmark_algorithms.h"

/******************************************************************************/
/******************************************************************************/

// this constant may need to be adjusted to give reasonable minimum times
// For best results, times should be about 1.0 seconds for the minimum test run
int iterations = 1200000;


// 4000 items, or about 32k of data
// this is intended to remain within the L2 cache of most common CPUs
const int SIZE = 4000;


// initial value for filling our arrays, may be changed from the command line
int32_t init_value = 3;

/******************************************************************************/

// our global arrays of numbers

int16_t data16[SIZE];
int16_t data16B[SIZE];

int32_t data32[SIZE];
int32_t data32B[SIZE];

int64_t data64[SIZE];
int64_t data64B[SIZE];

float dataFloat[SIZE];
float dataFloatB[SIZE];

double dataDouble[SIZE];
double dataDoubleB[SIZE];

/******************************************************************************/
/******************************************************************************/

template<typename T>
inline void check_sum(T result) {
    if ( fabs( result - (T(init_value)*T(init_value)*SIZE ) ) > 1.0e-6 )
        printf("test %i failed\n", current_test);
}

// specialization to avoid confusion about which fabs() to call with integers
template <>
inline void check_sum(uint64_t result) {
    uint64_t target =uint64_t(init_value)*uint64_t(init_value)*SIZE;
    if ( result != target )
        printf("test %i failed\n", current_test);
}

template <>
inline void check_sum(int64_t result) {
    int64_t target =int64_t(init_value)*int64_t(init_value)*SIZE;
    if ( result != target )
        printf("test %i failed\n", current_test);
}

template <>
inline void check_sum(uint32_t result) {
    uint32_t target =uint32_t(init_value)*uint32_t(init_value)*SIZE;
    if ( result != target )
        printf("test %i failed\n", current_test);
}

template <>
inline void check_sum(int32_t result) {
    int32_t target =int32_t(init_value)*int32_t(init_value)*SIZE;
    if ( result != target )
        printf("test %i failed\n", current_test);
}

template <>
inline void check_sum(uint16_t result) {
    uint16_t target =uint16_t(int32_t(init_value)*int32_t(init_value)*SIZE);
    if ( result != target )
        printf("test %i failed\n", current_test);
}

template <>
inline void check_sum(int16_t result) {
    int16_t target =int16_t(int32_t(init_value)*int32_t(init_value)*SIZE);
    if ( result != target )
        printf("test %i failed\n", current_test);
}

/******************************************************************************/
/******************************************************************************/

// baseline - a trivial multiplication loop
template <typename T>
void test_inner_product1(const T* first, const T* second, const int count, const char *label) {
    int i, j;

    start_timer();

    for(i = 0; i < iterations; ++i) {
        T sum( 0 );
        for (j = 0; j < count; ++j) {
            sum = sum + first[j] * second[j];
        }
        check_sum( sum );
    }

    record_result( timer(), label );
}

/******************************************************************************/

// STL inner_product template (iterator loop)
template <typename T>
void test_inner_product2(const T* first, const T* second, const int count, const char *label) {
    int i;

    start_timer();

    for(i = 0; i < iterations; ++i) {
        T sum( 0 );
        sum = std::inner_product( first, first+count, second, sum );
        check_sum( sum );
    }

    record_result( timer(), label );
}

/******************************************************************************/

// trivial iterator/pointer style loop
template <typename T>
void test_inner_product3(const T* first, const T* second, const int count, const char *label) {
    int i;

    start_timer();

    for(i = 0; i < iterations; ++i) {
        const T* currentA = first;
        const T* currentB = second;
        const T* endA = first + count;
        T sum( 0 );
        while (currentA != endA) {
            sum += *currentA * *currentB;
            currentA++;
            currentB++;
        }
        check_sum( sum );
    }

    record_result( timer(), label );
}

/******************************************************************************/

// unroll 2X
template <typename T>
void test_inner_product4(const T* first, const T* second, const int count, const char *label) {
    int i, j;

    start_timer();

    for(i = 0; i < iterations; ++i) {
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
        check_sum( sum );
    }

    record_result( timer(), label );
}

/******************************************************************************/

// unroll 4X
template <typename T>
void test_inner_product5(const T* first, const T* second, const int count, const char *label) {
    int i, j;

    start_timer();

    for(i = 0; i < iterations; ++i) {
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
        check_sum( sum );
    }

    record_result( timer(), label );
}

/******************************************************************************/

// unroll 2X with two accumulator variables
// allows simple vector (SSE/AVX) operations if source aligned
template <typename T>
void test_inner_product6(const T* first, const T* second, const int count, const char *label) {
    int i, j;

    start_timer();

    for(i = 0; i < iterations; ++i) {
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
        check_sum( sum );
    }

    record_result( timer(), label );
}

/******************************************************************************/

// unroll 4X with four accumulator variables
// allows simple vector (SSE/AVX) operations if source aligned
template <typename T>
void test_inner_product7(const T* first, const T* second, const int count, const char *label) {
    int i, j;

    start_timer();

    for(i = 0; i < iterations; ++i) {
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
        check_sum( sum );
    }

    record_result( timer(), label );
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

    fill(data16, data16+SIZE, int16_t(init_value));
    fill(data16B, data16B+SIZE, int16_t(init_value));
    fill(data32, data32+SIZE, int32_t(init_value));
    fill(data32B, data32B+SIZE, int32_t(init_value));
    fill(data64, data64+SIZE, int64_t(init_value));
    fill(data64B, data64B+SIZE, int64_t(init_value));
    fill(dataFloat, dataFloat+SIZE, float(init_value));
    fill(dataFloatB, dataFloatB+SIZE, float(init_value));
    fill(dataDouble, dataDouble+SIZE, double(init_value));
    fill(dataDoubleB, dataDoubleB+SIZE, double(init_value));


// int16_t
    test_inner_product1( data16, data16B, SIZE, "int16_t inner product simple");
    test_inner_product2( data16, data16B, SIZE, "int16_t inner product STL template");
    test_inner_product3( data16, data16B, SIZE, "int16_t inner product simple iter");
    test_inner_product4( data16, data16B, SIZE, "int16_t inner product4");
    test_inner_product5( data16, data16B, SIZE, "int16_t inner product5");
    test_inner_product6( data16, data16B, SIZE, "int16_t inner product6");
    test_inner_product7( data16, data16B, SIZE, "int16_t inner product7");
    
    summarize("int16_t inner product", SIZE, iterations, kDontShowGMeans, kDontShowPenalty );


// uint16_t
    test_inner_product1( (uint16_t*)data16, (uint16_t*)data16B, SIZE, "uint16_t inner product simple");
    test_inner_product2( (uint16_t*)data16, (uint16_t*)data16B, SIZE, "uint16_t inner product STL template");
    test_inner_product3( (uint16_t*)data16, (uint16_t*)data16B, SIZE, "uint16_t inner product simple iter");
    test_inner_product4( (uint16_t*)data16, (uint16_t*)data16B, SIZE, "uint16_t inner product4");
    test_inner_product5( (uint16_t*)data16, (uint16_t*)data16B, SIZE, "uint16_t inner product5");
    test_inner_product6( (uint16_t*)data16, (uint16_t*)data16B, SIZE, "uint16_t inner product6");
    test_inner_product7( (uint16_t*)data16, (uint16_t*)data16B, SIZE, "uint16_t inner product7");
    
    summarize("uint16_t inner product", SIZE, iterations, kDontShowGMeans, kDontShowPenalty );


// int32_t
    test_inner_product1( data32, data32B, SIZE, "int32_t inner product simple");
    test_inner_product2( data32, data32B, SIZE, "int32_t inner product STL template");
    test_inner_product3( data32, data32B, SIZE, "int32_t inner product simple iter");
    test_inner_product4( data32, data32B, SIZE, "int32_t inner product4");
    test_inner_product5( data32, data32B, SIZE, "int32_t inner product5");
    test_inner_product6( data32, data32B, SIZE, "int32_t inner product6");
    test_inner_product7( data32, data32B, SIZE, "int32_t inner product7");
    
    summarize("int32_t inner product", SIZE, iterations, kDontShowGMeans, kDontShowPenalty );


// uint32_t
    test_inner_product1( (uint32_t*)data32, (uint32_t*)data32B, SIZE, "uint32_t inner product simple");
    test_inner_product2( (uint32_t*)data32, (uint32_t*)data32B, SIZE, "uint32_t inner product STL template");
    test_inner_product3( (uint32_t*)data32, (uint32_t*)data32B, SIZE, "uint32_t inner product simple iter");
    test_inner_product4( (uint32_t*)data32, (uint32_t*)data32B, SIZE, "uint32_t inner product4");
    test_inner_product5( (uint32_t*)data32, (uint32_t*)data32B, SIZE, "uint32_t inner product5");
    test_inner_product6( (uint32_t*)data32, (uint32_t*)data32B, SIZE, "uint32_t inner product6");
    test_inner_product7( (uint32_t*)data32, (uint32_t*)data32B, SIZE, "uint32_t inner product7");
    
    summarize("uint32_t inner product", SIZE, iterations, kDontShowGMeans, kDontShowPenalty );


// int64_t
    test_inner_product1( data64, data64B, SIZE, "int64_t inner product simple");
    test_inner_product2( data64, data64B, SIZE, "int64_t inner product STL template");
    test_inner_product3( data64, data64B, SIZE, "int64_t inner product simple iter");
    test_inner_product4( data64, data64B, SIZE, "int64_t inner product4");
    test_inner_product5( data64, data64B, SIZE, "int64_t inner product5");
    test_inner_product6( data64, data64B, SIZE, "int64_t inner product6");
    test_inner_product7( data64, data64B, SIZE, "int64_t inner product7");

    summarize("int64_t inner product", SIZE, iterations, kDontShowGMeans, kDontShowPenalty );


// uint64_t
    test_inner_product1( (uint64_t*)data64, (uint64_t*)data64B, SIZE, "uint64_t inner product simple");
    test_inner_product2( (uint64_t*)data64, (uint64_t*)data64B, SIZE, "uint64_t inner product STL template");
    test_inner_product3( (uint64_t*)data64, (uint64_t*)data64B, SIZE, "uint64_t inner product simple iter");
    test_inner_product4( (uint64_t*)data64, (uint64_t*)data64B, SIZE, "uint64_t inner product4");
    test_inner_product5( (uint64_t*)data64, (uint64_t*)data64B, SIZE, "uint64_t inner product5");
    test_inner_product6( (uint64_t*)data64, (uint64_t*)data64B, SIZE, "uint64_t inner product6");
    test_inner_product7( (uint64_t*)data64, (uint64_t*)data64B, SIZE, "uint64_t inner product7");

    summarize("uint64_t inner product", SIZE, iterations, kDontShowGMeans, kDontShowPenalty );

    
// float
    test_inner_product1( dataFloat, dataFloatB, SIZE, "float inner product simple");
    test_inner_product2( dataFloat, dataFloatB, SIZE, "float inner product STL template");
    test_inner_product3( dataFloat, dataFloatB, SIZE, "float inner product simple iter");
    test_inner_product4( dataFloat, dataFloatB, SIZE, "float inner product4");
    test_inner_product5( dataFloat, dataFloatB, SIZE, "float inner product5");
    test_inner_product6( dataFloat, dataFloatB, SIZE, "float inner product6");
    test_inner_product7( dataFloat, dataFloatB, SIZE, "float inner product7");
    
    summarize("float inner product", SIZE, iterations, kDontShowGMeans, kDontShowPenalty );


// double
    test_inner_product1( dataDouble, dataDoubleB, SIZE, "double inner product simple");
    test_inner_product2( dataDouble, dataDoubleB, SIZE, "double inner product STL template");
    test_inner_product3( dataDouble, dataDoubleB, SIZE, "double inner product simple iter");
    test_inner_product4( dataDouble, dataDoubleB, SIZE, "double inner product4");
    test_inner_product5( dataDouble, dataDoubleB, SIZE, "double inner product5");
    test_inner_product6( dataDouble, dataDoubleB, SIZE, "double inner product6");
    test_inner_product7( dataDouble, dataDoubleB, SIZE, "double inner product7");

    summarize("double inner product", SIZE, iterations, kDontShowGMeans, kDontShowPenalty );



    return 0;
}

// the end
/******************************************************************************/
/******************************************************************************/
