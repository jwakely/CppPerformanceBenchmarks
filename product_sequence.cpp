/*
    Copyright 2008 Adobe Systems Incorporated
    Copyright 2019 Chris Cox
    Distributed under the MIT License (see accompanying file LICENSE_1_0_0.txt
    or a copy at http://stlab.adobe.com/licenses.html )


Goal: Test performance of various idioms for calculating the product of a sequence.


Assumptions:
    1) The compiler will optimize product operations.
    
    2) The compiler may recognize ineffecient product idioms and substitute efficient methods.


*/

/******************************************************************************/

#include "benchmark_stdint.hpp"
#include <cstddef>
#include <cstdio>
#include <ctime>
#include <cmath>
#include <cstdlib>
#include <string>
#include <deque>
#include <memory>
#include "benchmark_results.h"
#include "benchmark_timer.h"
#include "benchmark_algorithms.h"
#include "benchmark_typenames.h"

/******************************************************************************/
/******************************************************************************/

// this constant may need to be adjusted to give reasonable minimum times
// For best results, times should be about 1.0 seconds for the minimum test run
int iterations = 2000000;


// 4000 items, or about 32k of data
// this is intended to remain within the L2 cache of most common CPUs
const int SIZE = 4000;


// initial value for filling our arrays, may be changed from the command line
double init_value = 2.1;

/******************************************************************************/

std::deque<std::string> gLabels;

/******************************************************************************/
/******************************************************************************/

template<typename T>
inline void check_product(T result) {
  if ( fabs( result - pow(init_value,SIZE) ) > 1.0e-6 ) printf("test %i failed\n", current_test);
}

/******************************************************************************/
/******************************************************************************/

// baseline - a trivial multiplication loop
template <typename T>
T product1(const T* first, const int count, T initial) {
    T product( initial );
    
    for (int j = 0; j < count; ++j) {
        product = product * first[j];
    }
    
    return product;
}

/******************************************************************************/

// iterator style loop
template <typename T>
T product2(const T* first, const int count, T initial) {
    T product( initial );
    const T* current = first;
    const T* end = first+count;
    
    while (current != end) {
        product *= *current++;
    }
    
    return product;
}

/******************************************************************************/

// unroll 2X
template <typename T>
T product3(const T* first, const int count, T initial) {
    T product( initial );
    int j;
    
    for (j = 0; j < (count - 1); j += 2) {
        product *= first[j+0];
        product *= first[j+1];
    }

    for (; j < count; ++j) {
        product *= first[j];
    }
    
    return product;
}

/******************************************************************************/

// unroll 4X
template <typename T>
T product4(const T* first, const int count, T initial) {
    T product( initial );
    int j;
    
    for (j = 0; j < (count - 3); j += 4) {
        product *= first[j+0];
        product *= first[j+1];
        product *= first[j+2];
        product *= first[j+3];
    }

    for (; j < count; ++j) {
        product *= first[j];
    }
    
    return product;
}

/******************************************************************************/

// unroll 2X with multiple accumulator variables
template <typename T>
T product5(const T* first, const int count, T initial) {
    T product( initial );
    T product1(1);
    int j;
    
    for (j = 0; j < (count - 1); j += 2) {
        product  *= first[j+0];
        product1 *= first[j+1];
    }

    for (; j < count; ++j) {
        product *= first[j];
    }
    
    return product * product1;
}

/******************************************************************************/

// unroll 4X with multiple accumulator variables
template <typename T>
T product6(const T* first, const int count, T initial) {
    T product( initial );
    T product1( 1 );
    T product2( 1 );
    T product3( 1 );
    int j;
    
    for (j = 0; j < (count - 3); j += 4) {
        product  *= first[j+0];
        product1 *= first[j+1];
        product2 *= first[j+2];
        product3 *= first[j+3];
    }

    for (; j < count; ++j) {
        product *= first[j];
    }
    
    product *= product1 * product2 * product3;
    
    return product;
}

/******************************************************************************/

// unroll 4X and make it look like a vector operation
template <typename T>
T product7(const T* first, const int count, T initial) {
    T product[4] = { initial,1,1,1 };
    int j;
    
    for (j = 0; j < (count - 3); j += 4) {
        product[0] *= first[j+0];
        product[1] *= first[j+1];
        product[2] *= first[j+2];
        product[3] *= first[j+3];
    }

    for (; j < count; ++j) {
        product[0] *= first[j];
    }

    product[0] *= product[1] * product[2] * product[3];
    
    return product[0];
}

/******************************************************************************/

// unroll 8X and make it look like a vector operation
template <typename T>
T product8(const T* first, const int count, T initial) {
    T product[8] = { initial,1,1,1,1,1,1,1 };
    int j;
    
    for (j = 0; j < (count - 7); j += 8) {
        product[0] *= first[j+0];
        product[1] *= first[j+1];
        product[2] *= first[j+2];
        product[3] *= first[j+3];
        product[4] *= first[j+4];
        product[5] *= first[j+5];
        product[6] *= first[j+6];
        product[7] *= first[j+7];
    }

    for (; j < count; ++j) {
        product[0] *= first[j];
    }
    
    product[0] *= product[1] * product[2] * product[3];
    product[4] *= product[5] * product[6] * product[7];
    product[0] *= product[4];
    
    return product[0];
}

/******************************************************************************/
/******************************************************************************/

template <typename T, typename MM>
void testOneFunction(const T* first, const int count, MM func, const std::string label) {
    int i;

    start_timer();

    for(i = 0; i < iterations; ++i) {
    
        T result = func(first,count,T(1));
        
        check_product( result );
    }

    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

template<typename T>
void TestOneType()
{
    std::string myTypeName( getTypeName<T>() );

    std::unique_ptr<T> data_unique( new T[ SIZE ] );
    T *data = data_unique.get();
    
    fill(data, data+SIZE, T(init_value));
    
    testOneFunction( data, SIZE, product1<T>, myTypeName + " product sequence1" );
    testOneFunction( data, SIZE, product2<T>, myTypeName + " product sequence2" );
    testOneFunction( data, SIZE, product3<T>, myTypeName + " product sequence3" );
    testOneFunction( data, SIZE, product4<T>, myTypeName + " product sequence4" );
    testOneFunction( data, SIZE, product5<T>, myTypeName + " product sequence5" );
    testOneFunction( data, SIZE, product6<T>, myTypeName + " product sequence6" );
    testOneFunction( data, SIZE, product7<T>, myTypeName + " product sequence7" );
    testOneFunction( data, SIZE, product8<T>, myTypeName + " product sequence8" );

    std::string temp1( myTypeName + " product sequence" );
    summarize( temp1.c_str(), SIZE, iterations, kDontShowGMeans, kDontShowPenalty );

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
    if (argc > 2) init_value = (double) atof(argv[2]);


    TestOneType<float>();

    TestOneType<double>();


    return 0;
}

// the end
/******************************************************************************/
/******************************************************************************/
