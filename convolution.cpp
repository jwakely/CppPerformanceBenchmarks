/*
    Copyright 2008 Adobe Systems Incorporated
    Copyright 2018-2019 Chris Cox
    Distributed under the MIT License (see accompanying file LICENSE_1_0_0.txt
    or a copy at http://stlab.adobe.com/licenses.html )


Goal:  Test compiler optimizations related to convolution operations
        (common in imaging, sound, simulation, and scientific computations)
 


Assumptions:

    1) Most likely, there will be no single best implementation for all types.
        "Best" performance depends a lot on instruction latencies and register availability.

    2) Compilers will recognize inefficient loop orders and reorder loops for better cache usage.
 




TODO -
    repeat edge values
    2D with negative weights
    3D convolution
    3D as separable filters

    1D and separable with arbitrary weights passed in   -- separate file? convolution_arbitrary.cpp
    1D, 2D, 3D tent filters  -- separate file? convolution_tent.cpp
        iterated tent filters
    Difference of gaussians  -- separate file?

*/

#include "benchmark_stdint.hpp"
#include <cstddef>
#include <cstdio>
#include <ctime>
#include <cstdlib>
#include <cmath>
#include <deque>
#include <string>
#include <memory>
#include "benchmark_results.h"
#include "benchmark_timer.h"
#include "benchmark_typenames.h"

/******************************************************************************/

// this constant may need to be adjusted to give reasonable minimum times
// For best results, times should be about 1.0 seconds for the minimum test run
int iterations = 400;


// ~ 9 million items (src plus dest), intended to be larger than L2 cache on common CPUs
const int WIDTH = 1500;
const int HEIGHT = 3000;

const int SIZE = HEIGHT*WIDTH;


// initial value for filling our arrays, may be changed from the command line
double init_value = 3.0;

/******************************************************************************/

std::deque<std::string> gLabels;

/******************************************************************************/

#include "benchmark_shared_tests.h"

/******************************************************************************/

template <typename T>
inline void check_add_1D(const int edge, const T* out,
                        const int cols, const std::string &label) {
    const int edgeOffset = 2*edge;
    
    T sum = 0;
    int i;
    for (i = edge; i < (cols-edge); ++i)
        sum += out[i];
    
    T temp = (T)(SIZE - edgeOffset) * (T)init_value;
    if (!tolerance_equal<T>(sum,temp))
        printf("test %s failed\n", label.c_str() );
}

/******************************************************************************/

template <typename T>
inline void check_add_2D(const int edge, const T* out,
                        const int rows, const int cols,
                        const int rowStep, const std::string &label) {
    const int edgeOffset = (2*edge)*cols+(2*edge)*(rows-(2*edge));
    
    T sum = 0;
    for (int y = edge; y < (rows-edge); ++ y)
        for (int x = edge; x < (cols-edge); ++x)
            sum += out[(y*rowStep)+x];
    
    T temp = (T)(SIZE - edgeOffset) * (T)init_value;
    if (!tolerance_equal<T>(sum,temp))
        printf("test %s failed\n", label.c_str() );
}

/******************************************************************************/
/******************************************************************************/

/*
    1D convolution
    hard coded filter
    ignoring edges
    
    1 3 8 3 1
    result divided by 16
 
*/
template <typename T, typename TS >
void convolution1D(const T *source, T *dest, int cols, const std::string &label) {
    int i;
    const bool isFloat = (T(2.9) > T(2.0));
    const TS half = isFloat ? TS(0) : TS(8);

    start_timer();

    for(i = 0; i < iterations; ++i) {
        for (int x = 2; x < (cols-2); ++x) {
            TS sum = TS(1) * source[x-2] + TS(3) * source[x-1] + TS(8) * source[x+0] + TS(3) * source[x+1] + TS(1) * source[x+2];
            T temp = T( (sum+half) / TS(16) );
            dest[x] = temp;
        }
    }
    check_add_1D( 2, dest, cols, label );
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

// run the loop in reverse
template <typename T, typename TS >
void convolution1D_reverse(const T *source, T *dest, int cols, const std::string &label) {
    int i;
    const bool isFloat = (T(2.9) > T(2.0));
    const TS half = isFloat ? TS(0) : TS(8);

    start_timer();

    for(i = 0; i < iterations; ++i) {
        for (int x = (cols-3); x > 1 ; --x) {
            TS sum = TS(1) * source[x-2] + TS(3) * source[x-1] + TS(8) * source[x+0] + TS(3) * source[x+1] + TS(1) * source[x+2];
            T temp = T( (sum+half) / TS(16) );
            dest[x] = temp;
        }
    }
    check_add_1D( 2, dest, cols, label );
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

// rotate the values so they are only read once
template <typename T, typename TS >
void convolution1D_opt1(const T *source, T *dest, int cols, const std::string &label) {
    int i;
    const bool isFloat = (T(2.9) > T(2.0));
    const TS half = isFloat ? TS(0) : TS(8);

    start_timer();

    for(i = 0; i < iterations; ++i) {
        int x;

        T sourceXminus2 = source[2-2];
        T sourceXminus1 = source[2-1];
        T sourceXplus0 = source[2+0];
        T sourceXplus1 = source[2+1];

        for (x = 2; x < (cols-2); ++x) {
            T sourceXplus2 = source[x+2];
            TS sum = TS(1) * sourceXminus2 + TS(3) * sourceXminus1 + TS(8) * sourceXplus0 + TS(3) * sourceXplus1 + TS(1) * sourceXplus2;
            T temp = T( (sum+half) / TS(16) );
            sourceXminus2 = sourceXminus1;
            sourceXminus1 = sourceXplus0;
            sourceXplus0 = sourceXplus1;
            sourceXplus1 = sourceXplus2;
            dest[x] = temp;
        }

    }
    check_add_1D( 2, dest, cols, label );
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

// unroll the loop 4X
template <typename T, typename TS >
void convolution1D_opt2(const T *source, T *dest, int cols, const std::string &label) {
    int i;
    const bool isFloat = (T(2.9) > T(2.0));
    const TS half = isFloat ? TS(0) : TS(8);

    start_timer();

    for(i = 0; i < iterations; ++i) {
        int x;

        for (x = 2; x < (cols-2-3); x += 4) {
            T sourceXminus1 = source[x-1];
            T sourceXplus0 = source[x+0];
            T sourceXplus1 = source[x+1];
            T sourceXplus2 = source[x+2];
            T sourceXplus3 = source[x+3];
            T sourceXplus4 = source[x+4];
            
            TS sum0 = TS(1) * source[x-2] + TS(3) * sourceXminus1 + TS(8) * sourceXplus0 + TS(3) * sourceXplus1 + TS(1) * sourceXplus2;
            TS sum1 = TS(1) * sourceXminus1 + TS(3) * sourceXplus0 + TS(8) * sourceXplus1 + TS(3) * sourceXplus2 + TS(1) * sourceXplus3;
            TS sum2 = TS(1) * sourceXplus0 + TS(3) * sourceXplus1 + TS(8) * sourceXplus2 + TS(3) * sourceXplus3 + TS(1) * sourceXplus4;
            TS sum3 = TS(1) * sourceXplus1 + TS(3) * sourceXplus2 + TS(8) * sourceXplus3 + TS(3) * sourceXplus4 + TS(1) * source[x+5];
            
            T temp0 = T( (sum0+half) / TS(16) );
            T temp1 = T( (sum1+half) / TS(16) );
            T temp2 = T( (sum2+half) / TS(16) );
            T temp3 = T( (sum3+half) / TS(16) );
            
            dest[x+0] = temp0;
            dest[x+1] = temp1;
            dest[x+2] = temp2;
            dest[x+3] = temp3;
        }

        for (; x < (cols-2); ++x) {
            TS sum = TS(1) * source[x-2] + TS(3) * source[x-1] + TS(8) * source[x+0] + TS(3) * source[x+1] + TS(1) * source[x+2];
            T temp = T( (sum+half) / TS(16) );
            dest[x] = temp;
        }

    }
    check_add_1D( 2, dest, cols, label );
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

// unroll 4X
// change load style a little (seems to tickle a compiler oddity in gcc)
template <typename T, typename TS >
void convolution1D_opt3(const T *source, T *dest, int cols, const std::string &label) {
    int i;
    const bool isFloat = (T(2.9) > T(2.0));
    const TS half = isFloat ? TS(0) : TS(8);

    start_timer();

    for(i = 0; i < iterations; ++i) {
        int x;
        
        for (x = 2; x < (cols-2-3); x += 4) {
            T sourceXminus2 = source[x-2];
            T sourceXminus1 = source[x-1];
            T sourceXplus0 = source[x+0];
            T sourceXplus1 = source[x+1];
            T sourceXplus2 = source[x+2];
            T sourceXplus3 = source[x+3];
            T sourceXplus4 = source[x+4];
            T sourceXplus5 = source[x+5];
            
            TS sum0 = TS(1) * sourceXminus2 + TS(3) * sourceXminus1 + TS(8) * sourceXplus0 + TS(3) * sourceXplus1 + TS(1) * sourceXplus2;
            TS sum1 = TS(1) * sourceXminus1 + TS(3) * sourceXplus0 + TS(8) * sourceXplus1 + TS(3) * sourceXplus2 + TS(1) * sourceXplus3;
            TS sum2 = TS(1) * sourceXplus0 + TS(3) * sourceXplus1 + TS(8) * sourceXplus2 + TS(3) * sourceXplus3 + TS(1) * sourceXplus4;
            TS sum3 = TS(1) * sourceXplus1 + TS(3) * sourceXplus2 + TS(8) * sourceXplus3 + TS(3) * sourceXplus4 + TS(1) * sourceXplus5;
            
            T temp0 = T( (sum0+half) / TS(16) );
            T temp1 = T( (sum1+half) / TS(16) );
            T temp2 = T( (sum2+half) / TS(16) );
            T temp3 = T( (sum3+half) / TS(16) );
            
            dest[x+0] = temp0;
            dest[x+1] = temp1;
            dest[x+2] = temp2;
            dest[x+3] = temp3;
        }

        for (; x < (cols-2); ++x) {
            T sourceXminus2 = source[x-2];
            T sourceXminus1 = source[x-1];
            T sourceXplus0 = source[x+0];
            T sourceXplus1 = source[x+1];
            T sourceXplus2 = source[x+2];
            TS sum = TS(1) * sourceXminus2 + TS(3) * sourceXminus1 + TS(8) * sourceXplus0 + TS(3) * sourceXplus1 + TS(1) * sourceXplus2;
            T temp = T( (sum+half) / TS(16) );
            dest[x] = temp;
        }
    }
    check_add_1D( 2, dest, cols, label );
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

// unroll 4X, and rotate values
template <typename T, typename TS >
void convolution1D_opt4(const T *source, T *dest, int cols, const std::string &label) {
    int i;
    const bool isFloat = (T(2.9) > T(2.0));
    const TS half = isFloat ? TS(0) : TS(8);

    start_timer();

    for(i = 0; i < iterations; ++i) {
        int x;

        T sourceXminus2 = source[2-2];
        T sourceXminus1 = source[2-1];
        T sourceXplus0 = source[2+0];
        T sourceXplus1 = source[2+1];
        
        for (x = 2; x < (cols-2-3); x += 4) {
            T sourceXplus2 = source[x+2];
            T sourceXplus3 = source[x+3];
            T sourceXplus4 = source[x+4];
            T sourceXplus5 = source[x+5];
            
            TS sum0 = TS(1) * sourceXminus2 + TS(3) * sourceXminus1 + TS(8) * sourceXplus0 + TS(3) * sourceXplus1 + TS(1) * sourceXplus2;
            TS sum1 = TS(1) * sourceXminus1 + TS(3) * sourceXplus0 + TS(8) * sourceXplus1 + TS(3) * sourceXplus2 + TS(1) * sourceXplus3;
            TS sum2 = TS(1) * sourceXplus0 + TS(3) * sourceXplus1 + TS(8) * sourceXplus2 + TS(3) * sourceXplus3 + TS(1) * sourceXplus4;
            TS sum3 = TS(1) * sourceXplus1 + TS(3) * sourceXplus2 + TS(8) * sourceXplus3 + TS(3) * sourceXplus4 + TS(1) * sourceXplus5;
            
            sourceXminus2 = sourceXplus2;
            sourceXminus1 = sourceXplus3;
            sourceXplus0 = sourceXplus4;
            sourceXplus1 = sourceXplus5;
            
            T temp0 = T( (sum0+half) / TS(16) );
            T temp1 = T( (sum1+half) / TS(16) );
            T temp2 = T( (sum2+half) / TS(16) );
            T temp3 = T( (sum3+half) / TS(16) );
            
            dest[x+0] = temp0;
            dest[x+1] = temp1;
            dest[x+2] = temp2;
            dest[x+3] = temp3;
        }

        for (; x < (cols-2); ++x) {
            T sourceXplus2 = source[x+2];
            TS sum = TS(1) * sourceXminus2 + TS(3) * sourceXminus1 + TS(8) * sourceXplus0 + TS(3) * sourceXplus1 + TS(1) * sourceXplus2;
            T temp = T( (sum+half) / TS(16) );
            sourceXminus2 = sourceXminus1;
            sourceXminus1 = sourceXplus0;
            sourceXplus0 = sourceXplus1;
            sourceXplus1 = sourceXplus2;
            dest[x] = temp;
        }
        
    }
    check_add_1D( 2, dest, cols, label );
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

// unroll 8X
template <typename T, typename TS >
void convolution1D_opt5(const T *source, T *dest, int cols, const std::string &label) {
    int i;
    const bool isFloat = (T(2.9) > T(2.0));
    const TS half = isFloat ? TS(0) : TS(8);

    start_timer();

    for(i = 0; i < iterations; ++i) {
        int x;
        
        // really should align to vector boundary here
        
        for (x = 2; x < (cols-2-7); x += 8) {
            
            T sourceXminus2 = source[x-2];
            T sourceXminus1 = source[x-1];
            T sourceXplus0 = source[x+0];
            T sourceXplus1 = source[x+1];
            T sourceXplus2 = source[x+2];
            T sourceXplus3 = source[x+3];
            T sourceXplus4 = source[x+4];
            T sourceXplus5 = source[x+5];
            T sourceXplus6 = source[x+6];
            T sourceXplus7 = source[x+7];
            T sourceXplus8 = source[x+8];
            T sourceXplus9 = source[x+9];
            
            TS sum0 = TS(1) * sourceXminus2 + TS(3) * sourceXminus1 + TS(8) * sourceXplus0 + TS(3) * sourceXplus1 + TS(1) * sourceXplus2;
            TS sum1 = TS(1) * sourceXminus1 + TS(3) * sourceXplus0 + TS(8) * sourceXplus1 + TS(3) * sourceXplus2 + TS(1) * sourceXplus3;
            TS sum2 = TS(1) * sourceXplus0 + TS(3) * sourceXplus1 + TS(8) * sourceXplus2 + TS(3) * sourceXplus3 + TS(1) * sourceXplus4;
            TS sum3 = TS(1) * sourceXplus1 + TS(3) * sourceXplus2 + TS(8) * sourceXplus3 + TS(3) * sourceXplus4 + TS(1) * sourceXplus5;
            TS sum4 = TS(1) * sourceXplus2 + TS(3) * sourceXplus3 + TS(8) * sourceXplus4 + TS(3) * sourceXplus5 + TS(1) * sourceXplus6;
            TS sum5 = TS(1) * sourceXplus3 + TS(3) * sourceXplus4 + TS(8) * sourceXplus5 + TS(3) * sourceXplus6 + TS(1) * sourceXplus7;
            TS sum6 = TS(1) * sourceXplus4 + TS(3) * sourceXplus5 + TS(8) * sourceXplus6 + TS(3) * sourceXplus7 + TS(1) * sourceXplus8;
            TS sum7 = TS(1) * sourceXplus5 + TS(3) * sourceXplus6 + TS(8) * sourceXplus7 + TS(3) * sourceXplus8 + TS(1) * sourceXplus9;
            
            T temp0 = T( (sum0+half) / TS(16) );
            T temp1 = T( (sum1+half) / TS(16) );
            T temp2 = T( (sum2+half) / TS(16) );
            T temp3 = T( (sum3+half) / TS(16) );
            T temp4 = T( (sum4+half) / TS(16) );
            T temp5 = T( (sum5+half) / TS(16) );
            T temp6 = T( (sum6+half) / TS(16) );
            T temp7 = T( (sum7+half) / TS(16) );
            
            dest[x+0] = temp0;
            dest[x+1] = temp1;
            dest[x+2] = temp2;
            dest[x+3] = temp3;
            dest[x+4] = temp4;
            dest[x+5] = temp5;
            dest[x+6] = temp6;
            dest[x+7] = temp7;
        }

        for (; x < (cols-2); ++x) {
            T sourceXminus2 = source[x-2];
            T sourceXminus1 = source[x-1];
            T sourceXplus0 = source[x+0];
            T sourceXplus1 = source[x+1];
            T sourceXplus2 = source[x+2];
            
            TS sum = TS(1) * sourceXminus2 + TS(3) * sourceXminus1 + TS(8) * sourceXplus0 + TS(3) * sourceXplus1 + TS(1) * sourceXplus2;
            T temp = T( (sum+half) / TS(16) );
            dest[x] = temp;
        }
        
    }
    check_add_1D( 2, dest, cols, label );
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

// unroll 8X, and rotate values
template <typename T, typename TS >
void convolution1D_opt6(const T *source, T *dest, int cols, const std::string &label) {
    int i;
    const bool isFloat = (T(2.9) > T(2.0));
    const TS half = isFloat ? TS(0) : TS(8);

    start_timer();

    for(i = 0; i < iterations; ++i) {
        int x;

        T sourceXminus2 = source[2-2];
        T sourceXminus1 = source[2-1];
        T sourceXplus0 = source[2+0];
        T sourceXplus1 = source[2+1];
        
        // really should align to vector boundary here
        
        for (x = 2; x < (cols-2-7); x += 8) {
            
            T sourceXplus2 = source[x+2];
            T sourceXplus3 = source[x+3];
            T sourceXplus4 = source[x+4];
            T sourceXplus5 = source[x+5];
            T sourceXplus6 = source[x+6];
            T sourceXplus7 = source[x+7];
            T sourceXplus8 = source[x+8];
            T sourceXplus9 = source[x+9];
            
            TS sum0 = TS(1) * sourceXminus2 + TS(3) * sourceXminus1 + TS(8) * sourceXplus0 + TS(3) * sourceXplus1 + TS(1) * sourceXplus2;
            TS sum1 = TS(1) * sourceXminus1 + TS(3) * sourceXplus0 + TS(8) * sourceXplus1 + TS(3) * sourceXplus2 + TS(1) * sourceXplus3;
            TS sum2 = TS(1) * sourceXplus0 + TS(3) * sourceXplus1 + TS(8) * sourceXplus2 + TS(3) * sourceXplus3 + TS(1) * sourceXplus4;
            TS sum3 = TS(1) * sourceXplus1 + TS(3) * sourceXplus2 + TS(8) * sourceXplus3 + TS(3) * sourceXplus4 + TS(1) * sourceXplus5;
            TS sum4 = TS(1) * sourceXplus2 + TS(3) * sourceXplus3 + TS(8) * sourceXplus4 + TS(3) * sourceXplus5 + TS(1) * sourceXplus6;
            TS sum5 = TS(1) * sourceXplus3 + TS(3) * sourceXplus4 + TS(8) * sourceXplus5 + TS(3) * sourceXplus6 + TS(1) * sourceXplus7;
            TS sum6 = TS(1) * sourceXplus4 + TS(3) * sourceXplus5 + TS(8) * sourceXplus6 + TS(3) * sourceXplus7 + TS(1) * sourceXplus8;
            TS sum7 = TS(1) * sourceXplus5 + TS(3) * sourceXplus6 + TS(8) * sourceXplus7 + TS(3) * sourceXplus8 + TS(1) * sourceXplus9;
            
            T temp0 = T( (sum0+half) / TS(16) );
            T temp1 = T( (sum1+half) / TS(16) );
            T temp2 = T( (sum2+half) / TS(16) );
            T temp3 = T( (sum3+half) / TS(16) );
            T temp4 = T( (sum4+half) / TS(16) );
            T temp5 = T( (sum5+half) / TS(16) );
            T temp6 = T( (sum6+half) / TS(16) );
            T temp7 = T( (sum7+half) / TS(16) );
            
            sourceXminus2 = sourceXplus6;
            sourceXminus1 = sourceXplus7;
            sourceXplus0 = sourceXplus8;
            sourceXplus1 = sourceXplus9;
            
            dest[x+0] = temp0;
            dest[x+1] = temp1;
            dest[x+2] = temp2;
            dest[x+3] = temp3;
            dest[x+4] = temp4;
            dest[x+5] = temp5;
            dest[x+6] = temp6;
            dest[x+7] = temp7;
        }

        for (; x < (cols-2); ++x) {
            T sourceXplus2 = source[x+2];
            TS sum = TS(1) * sourceXminus2 + TS(3) * sourceXminus1 + TS(8) * sourceXplus0 + TS(3) * sourceXplus1 + TS(1) * sourceXplus2;
            T temp = T( (sum+half) / TS(16) );
            sourceXminus2 = sourceXminus1;
            sourceXminus1 = sourceXplus0;
            sourceXplus0 = sourceXplus1;
            sourceXplus1 = sourceXplus2;
            dest[x] = temp;
        }

    }
    check_add_1D( 2, dest, cols, label );
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/
/******************************************************************************/

/*
    2D convolution
    hard coded filter
    ignoring edges
    
        1 5 1
        5 8 5
        1 5 1
    result divided by 32
*/
template <typename T, typename TS >
void convolution2D(const T *source, T *dest, int rows, int cols, int rowStep, const std::string &label) {
    int i;
    const bool isFloat = (T(2.9) > T(2.0));
    const TS half = isFloat ? TS(0) : TS(16);

    start_timer();

    for(i = 0; i < iterations; ++i) {
        int x, y;
        
        for (y = 1; y < (rows-1); ++ y) {

            for (x = 1; x < (cols-1); ++x) {
                TS sum = TS(1) * source[((y-1)*rowStep)+(x-1)] + TS(5) * source[((y-1)*rowStep)+(x+0)] + TS(1) * source[((y-1)*rowStep)+(x+1)];
                sum += TS(5) * source[((y+0)*rowStep)+(x-1)] + TS(8) * source[((y+0)*rowStep)+(x+0)] + TS(5) * source[((y+0)*rowStep)+(x+1)];
                sum += TS(1) * source[((y+1)*rowStep)+(x-1)] + TS(5) * source[((y+1)*rowStep)+(x+0)] + TS(1) * source[((y+1)*rowStep)+(x+1)];
                T temp = T( (sum+half) / TS(32) );
                dest[((y+0)*rowStep)+(x+0)] = temp;
            }
        
        }
    }
    check_add_2D(1,dest,rows,cols,rowStep,label);
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

// run the loops in reverse
template <typename T, typename TS >
void convolution2D_reverse(const T *source, T *dest, int rows, int cols, int rowStep, const std::string &label) {
    int i;
    const bool isFloat = (T(2.9) > T(2.0));
    const TS half = isFloat ? TS(0) : TS(16);

    start_timer();

    for(i = 0; i < iterations; ++i) {
        int x, y;
        
        for (y = (rows-2); y > 0; --y) {

            for (x = (cols-2); x > 0; --x) {
                TS sum = TS(1) * source[((y-1)*rowStep)+(x-1)] + TS(5) * source[((y-1)*rowStep)+(x+0)] + TS(1) * source[((y-1)*rowStep)+(x+1)];
                sum += TS(5) * source[((y+0)*rowStep)+(x-1)] + TS(8) * source[((y+0)*rowStep)+(x+0)] + TS(5) * source[((y+0)*rowStep)+(x+1)];
                sum += TS(1) * source[((y+1)*rowStep)+(x-1)] + TS(5) * source[((y+1)*rowStep)+(x+0)] + TS(1) * source[((y+1)*rowStep)+(x+1)];
                T temp = T( (sum+half) / TS(32) );
                dest[((y+0)*rowStep)+(x+0)] = temp;
            }
        
        }

    }
    check_add_2D(1,dest,rows,cols,rowStep,label);
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

// run X loop in reverse
template <typename T, typename TS >
void convolution2D_reverseX(const T *source, T *dest, int rows, int cols, int rowStep, const std::string &label) {
    int i;
    const bool isFloat = (T(2.9) > T(2.0));
    const TS half = isFloat ? TS(0) : TS(16);

    start_timer();

    for(i = 0; i < iterations; ++i) {
        int x, y;
        
        for (y = 1; y < (rows-1); ++ y) {

            for (x = (cols-2); x > 0; --x) {
                TS sum = TS(1) * source[((y-1)*rowStep)+(x-1)] + TS(5) * source[((y-1)*rowStep)+(x+0)] + TS(1) * source[((y-1)*rowStep)+(x+1)];
                sum += TS(5) * source[((y+0)*rowStep)+(x-1)] + TS(8) * source[((y+0)*rowStep)+(x+0)] + TS(5) * source[((y+0)*rowStep)+(x+1)];
                sum += TS(1) * source[((y+1)*rowStep)+(x-1)] + TS(5) * source[((y+1)*rowStep)+(x+0)] + TS(1) * source[((y+1)*rowStep)+(x+1)];
                T temp = T( (sum+half) / TS(32) );
                dest[((y+0)*rowStep)+(x+0)] = temp;
            }
        
        }

    }
    check_add_2D(1,dest,rows,cols,rowStep,label);
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

// run Y loop in reverse
template <typename T, typename TS >
void convolution2D_reverseY(const T *source, T *dest, int rows, int cols, int rowStep, const std::string &label) {
    int i;
    const bool isFloat = (T(2.9) > T(2.0));
    const TS half = isFloat ? TS(0) : TS(16);

    start_timer();

    for(i = 0; i < iterations; ++i) {
        int x, y;
        
        for (y = (rows-2); y > 0; --y) {

            for (x = 1; x < (cols-1); ++x) {
                TS sum = TS(1) * source[((y-1)*rowStep)+(x-1)] + TS(5) * source[((y-1)*rowStep)+(x+0)] + TS(1) * source[((y-1)*rowStep)+(x+1)];
                sum += TS(5) * source[((y+0)*rowStep)+(x-1)] + TS(8) * source[((y+0)*rowStep)+(x+0)] + TS(5) * source[((y+0)*rowStep)+(x+1)];
                sum += TS(1) * source[((y+1)*rowStep)+(x-1)] + TS(5) * source[((y+1)*rowStep)+(x+0)] + TS(1) * source[((y+1)*rowStep)+(x+1)];
                T temp = T( (sum+half) / TS(32) );
                dest[((y+0)*rowStep)+(x+0)] = temp;
            }
        
        }

    }
    check_add_2D(1,dest,rows,cols,rowStep,label);
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

// swap the loops
template <typename T, typename TS >
void convolution2D_loopswap(const T *source, T *dest, int rows, int cols, int rowStep, const std::string &label) {
    int i;
    const bool isFloat = (T(2.9) > T(2.0));
    const TS half = isFloat ? TS(0) : TS(16);

    start_timer();

    for(i = 0; i < iterations; ++i) {
        int x, y;
        
        for (x = 1; x < (cols-1); ++x) {
        
            for (y = 1; y < (rows-1); ++ y) {

                TS sum = TS(1) * source[((y-1)*rowStep)+(x-1)] + TS(5) * source[((y-1)*rowStep)+(x+0)] + TS(1) * source[((y-1)*rowStep)+(x+1)];
                sum += TS(5) * source[((y+0)*rowStep)+(x-1)] + TS(8) * source[((y+0)*rowStep)+(x+0)] + TS(5) * source[((y+0)*rowStep)+(x+1)];
                sum += TS(1) * source[((y+1)*rowStep)+(x-1)] + TS(5) * source[((y+1)*rowStep)+(x+0)] + TS(1) * source[((y+1)*rowStep)+(x+1)];
                T temp = T( (sum+half) / TS(32) );
                dest[((y+0)*rowStep)+(x+0)] = temp;
            }
        
        }
    }
    check_add_2D(1,dest,rows,cols,rowStep,label);
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

// move loop invariant values out (loop invariant code motion)
template <typename T, typename TS >
void convolution2D_opt1(const T *source, T *dest, int rows, int cols, int rowStep, const std::string &label) {
    int i;
    const bool isFloat = (T(2.9) > T(2.0));
    const TS half = isFloat ? TS(0) : TS(16);

    start_timer();

    for(i = 0; i < iterations; ++i) {
        int x, y;
        
        const T *sourceYminus1 = &source[((1-1)*rowStep)];
        const T *sourceYplus0 = &source[((1+0)*rowStep)];
        const T *sourceYplus1 = &source[((1+1)*rowStep)];
        T *dest2 = dest + rowStep;
        
        for (y = 1; y < (rows - 1); ++ y) {

            for (x = 1; x < (cols-1); ++x) {
                TS sum = TS(1) * sourceYminus1[(x-1)] + TS(5) * sourceYminus1[(x+0)] + TS(1) * sourceYminus1[(x+1)];
                sum += TS(5) * sourceYplus0[(x-1)] + TS(8) * sourceYplus0[(x+0)] + TS(5) * sourceYplus0[(x+1)];
                sum += TS(1) * sourceYplus1[(x-1)] + TS(5) * sourceYplus1[(x+0)] + TS(1) * sourceYplus1[(x+1)];
                T temp = T( (sum+half) / TS(32) );
                dest2[x+0] = temp;
            }
            
            sourceYminus1 += rowStep;
            sourceYplus0 += rowStep;
            sourceYplus1 += rowStep;
            dest2 += rowStep;
        }
    }
    check_add_2D(1,dest,rows,cols,rowStep,label);
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

// move loop invariant values out
// rotate the values so they are only read once
template <typename T, typename TS >
void convolution2D_opt2(const T *source, T *dest, int rows, int cols, int rowStep, const std::string &label) {
    int i;
    const bool isFloat = (T(2.9) > T(2.0));
    const TS half = isFloat ? TS(0) : TS(16);

    start_timer();

    for(i = 0; i < iterations; ++i) {
        int x, y;
        
        const T *sourceYminus1 = &source[((1-1)*rowStep)];
        const T *sourceYplus0 = &source[((1+0)*rowStep)];
        const T *sourceYplus1 = &source[((1+1)*rowStep)];
        T *dest2 = dest + rowStep;
        
        for (y = 1; y < (rows - 1); ++ y) {
            T sourceYminus1Xminus1 = sourceYminus1[(1-1)];
            T sourceYminus1Xplus0 = sourceYminus1[(1+0)];
            T sourceYplus0Xminus1 = sourceYplus0[(1-1)];
            T sourceYplus0Xplus0 = sourceYplus0[(1+0)];
            T sourceYplus1Xminus1 = sourceYplus1[(1-1)];
            T sourceYplus1Xplus0 =sourceYplus1[(1+0)];

            for (x = 1; x < (cols-1); ++x) {
                T sourceYminus1Xplus1 = sourceYminus1[(x+1)];
                T sourceYplus0Xplus1 = sourceYplus0[(x+1)];
                T sourceYplus1Xplus1 = sourceYplus1[(x+1)];
                
                TS sum = TS(1) * sourceYminus1Xminus1 + TS(5) * sourceYminus1Xplus0 + TS(1) * sourceYminus1Xplus1;
                sum += TS(5) * sourceYplus0Xminus1 + TS(8) * sourceYplus0Xplus0 + TS(5) * sourceYplus0Xplus1;
                sum += TS(1) * sourceYplus1Xminus1 + TS(5) * sourceYplus1Xplus0 + TS(1) * sourceYplus1Xplus1;
                
                T temp = T( (sum+half) / TS(32) );
                
                sourceYminus1Xminus1 = sourceYminus1Xplus0;
                sourceYminus1Xplus0 = sourceYminus1Xplus1;
                sourceYplus0Xminus1 = sourceYplus0Xplus0;
                sourceYplus0Xplus0 = sourceYplus0Xplus1;
                sourceYplus1Xminus1 = sourceYplus1Xplus0;
                sourceYplus1Xplus0 = sourceYplus1Xplus1;
                
                dest2[x+0] = temp;
            }
            
            sourceYminus1 += rowStep;
            sourceYplus0 += rowStep;
            sourceYplus1 += rowStep;
            dest2 += rowStep;
        }
    }
    check_add_2D(1,dest,rows,cols,rowStep,label);
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

// move loop invariant values out
// collect common terms (subexpressions), rotate them between loops
template <typename T, typename TS >
void convolution2D_opt3(const T *source, T *dest, int rows, int cols, int rowStep, const std::string &label) {
    int i;
    const bool isFloat = (T(2.9) > T(2.0));
    const TS half = isFloat ? TS(0) : TS(16);

    start_timer();

    for(i = 0; i < iterations; ++i) {
        int x, y;
        
        const T *sourceYminus1 = &source[((1-1)*rowStep)];
        const T *sourceYplus0 = &source[((1+0)*rowStep)];
        const T *sourceYplus1 = &source[((1+1)*rowStep)];
        T *dest2 = dest + rowStep;
        
        for (y = 1; y < (rows - 1); ++ y) {

            T sourceYminus1Xminus1 = sourceYminus1[(1-1)];
            T sourceYplus0Xminus1 = sourceYplus0[(1-1)];
            T sourceYplus1Xminus1 = sourceYplus1[(1-1)];
            
            TS tempColN1 = TS(1) * sourceYminus1Xminus1 + TS(5) * sourceYplus0Xminus1 + TS(1) * sourceYplus1Xminus1;
            
            T sourceYminus1Xplus0 = sourceYminus1[(1+0)];
            T sourceYplus0Xplus0 = sourceYplus0[(1+0)];
            T sourceYplus1Xplus0 = sourceYplus1[(1+0)];
            
            TS tempCol0 = TS(1) * sourceYminus1Xplus0 + TS(5) * sourceYplus0Xplus0 + TS(1) * sourceYplus1Xplus0;

            for ( x = 1; x < (cols-1); ++x ) {
                
                T sourceYminus1Xplus0 = sourceYminus1[(x+0)];
                T sourceYplus0Xplus0 = sourceYplus0[(x+0)];
                T sourceYplus1Xplus0 = sourceYplus1[(x+0)];
                
                T sourceYminus1Xplus1 = sourceYminus1[(x+1)];
                T sourceYplus0Xplus1 = sourceYplus0[(x+1)];
                T sourceYplus1Xplus1 = sourceYplus1[(x+1)];
                
                TS tempCol1 = TS(1) * sourceYminus1Xplus1 + TS(5) * sourceYplus0Xplus1 + TS(1) * sourceYplus1Xplus1;
                
                TS sum = TS(5) * sourceYminus1Xplus0 + tempColN1 + tempCol1;
                sum += TS(8) * sourceYplus0Xplus0;
                sum += TS(5) * sourceYplus1Xplus0;
                
                tempColN1 = tempCol0;
                tempCol0 = tempCol1;
                
                T temp = T( (sum+half) / T(32) );
                dest2[x+0] = temp;
            }
            
            sourceYminus1 += rowStep;
            sourceYplus0 += rowStep;
            sourceYplus1 += rowStep;
            dest2 += rowStep;
        }

    }
    check_add_2D(1,dest,rows,cols,rowStep,label);
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}


/******************************************************************************/

// move loop invariant values out
// unroll 4x
template <typename T, typename TS >
void convolution2D_opt4(const T *source, T *dest, int rows, int cols, int rowStep, const std::string &label) {
    int i;
    const bool isFloat = (T(2.9) > T(2.0));
    const TS half = isFloat ? TS(0) : TS(16);

    start_timer();

    for(i = 0; i < iterations; ++i) {
        int x, y;
        
        const T *sourceYminus1 = &source[((1-1)*rowStep)];
        const T *sourceYplus0 = &source[((1+0)*rowStep)];
        const T *sourceYplus1 = &source[((1+1)*rowStep)];
        T *dest2 = dest + rowStep;
        
        for (y = 1; y < (rows - 1); ++ y) {

            for (x = 1; x < ((cols-1)-3); x += 4) {
                TS sum0 = TS(1) * sourceYminus1[(x-1)] + TS(5) * sourceYminus1[(x+0)] + TS(1) * sourceYminus1[(x+1)];
                sum0 += TS(5) * sourceYplus0[(x-1)] + TS(8) * sourceYplus0[(x+0)] + TS(5) * sourceYplus0[(x+1)];
                sum0 += TS(1) * sourceYplus1[(x-1)] + TS(5) * sourceYplus1[(x+0)] + TS(1) * sourceYplus1[(x+1)];
                
                TS sum1 = TS(1) * sourceYminus1[(x+0)] + TS(5) * sourceYminus1[(x+1)] + TS(1) * sourceYminus1[(x+2)];
                sum1 += TS(5) * sourceYplus0[(x+0)] + TS(8) * sourceYplus0[(x+1)] + TS(5) * sourceYplus0[(x+2)];
                sum1 += TS(1) * sourceYplus1[(x+0)] + TS(5) * sourceYplus1[(x+1)] + TS(1) * sourceYplus1[(x+2)];
                
                TS sum2 = TS(1) * sourceYminus1[(x+1)] + TS(5) * sourceYminus1[(x+2)] + TS(1) * sourceYminus1[(x+3)];
                sum2 += TS(5) * sourceYplus0[(x+1)] + TS(8) * sourceYplus0[(x+2)] + TS(5) * sourceYplus0[(x+3)];
                sum2 += TS(1) * sourceYplus1[(x+1)] + TS(5) * sourceYplus1[(x+2)] + TS(1) * sourceYplus1[(x+3)];
                
                TS sum3 = TS(1) * sourceYminus1[(x+2)] + TS(5) * sourceYminus1[(x+3)] + TS(1) * sourceYminus1[(x+4)];
                sum3 += TS(5) * sourceYplus0[(x+2)] + TS(8) * sourceYplus0[(x+3)] + TS(5) * sourceYplus0[(x+4)];
                sum3 += TS(1) * sourceYplus1[(x+2)] + TS(5) * sourceYplus1[(x+3)] + TS(1) * sourceYplus1[(x+4)];
                
                T temp0 = T( (sum0+half) / T(32) );
                T temp1 = T( (sum1+half) / T(32) );
                T temp2 = T( (sum2+half) / T(32) );
                T temp3 = T( (sum3+half) / T(32) );
                
                dest2[x+0] = temp0;
                dest2[x+1] = temp1;
                dest2[x+2] = temp2;
                dest2[x+3] = temp3;
            }

            for ( ; x < (cols-1); ++x ) {
                TS sum = TS(1) * sourceYminus1[(x-1)] + TS(5) * sourceYminus1[(x+0)] + TS(1) * sourceYminus1[(x+1)];
                sum += TS(5) * sourceYplus0[(x-1)] + TS(8) * sourceYplus0[(x+0)] + TS(5) * sourceYplus0[(x+1)];
                sum += TS(1) * sourceYplus1[(x-1)] + TS(5) * sourceYplus1[(x+0)] + TS(1) * sourceYplus1[(x+1)];
                T temp = T( (sum+half) / T(32) );
                dest2[x+0] = temp;
            }
            
            sourceYminus1 += rowStep;
            sourceYplus0 += rowStep;
            sourceYplus1 += rowStep;
            dest2 += rowStep;
        }

    }
    check_add_2D(1,dest,rows,cols,rowStep,label);
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

// move loop invariant values out
// unroll 8x
template <typename T, typename TS >
void convolution2D_opt5(const T *source, T *dest, int rows, int cols, int rowStep, const std::string &label) {
    int i;
    const bool isFloat = (T(2.9) > T(2.0));
    const TS half = isFloat ? TS(0) : TS(16);

    start_timer();

    for(i = 0; i < iterations; ++i) {
        int x, y;
        
        const T *sourceYminus1 = &source[((1-1)*rowStep)];
        const T *sourceYplus0 = &source[((1+0)*rowStep)];
        const T *sourceYplus1 = &source[((1+1)*rowStep)];
        T *dest2 = dest + rowStep;
        
        for (y = 1; y < (rows - 1); ++ y) {

            for (x = 1; x < ((cols-1)-7); x += 8) {
                TS sum0 = TS(1) * sourceYminus1[(x-1)] + TS(5) * sourceYminus1[(x+0)] + TS(1) * sourceYminus1[(x+1)];
                sum0 += TS(5) * sourceYplus0[(x-1)] + TS(8) * sourceYplus0[(x+0)] + TS(5) * sourceYplus0[(x+1)];
                sum0 += TS(1) * sourceYplus1[(x-1)] + TS(5) * sourceYplus1[(x+0)] + TS(1) * sourceYplus1[(x+1)];
                
                TS sum1 = TS(1) * sourceYminus1[(x+0)] + TS(5) * sourceYminus1[(x+1)] + TS(1) * sourceYminus1[(x+2)];
                sum1 += TS(5) * sourceYplus0[(x+0)] + TS(8) * sourceYplus0[(x+1)] + TS(5) * sourceYplus0[(x+2)];
                sum1 += TS(1) * sourceYplus1[(x+0)] + TS(5) * sourceYplus1[(x+1)] + TS(1) * sourceYplus1[(x+2)];
                
                TS sum2 = TS(1) * sourceYminus1[(x+1)] + TS(5) * sourceYminus1[(x+2)] + TS(1) * sourceYminus1[(x+3)];
                sum2 += TS(5) * sourceYplus0[(x+1)] + TS(8) * sourceYplus0[(x+2)] + TS(5) * sourceYplus0[(x+3)];
                sum2 += TS(1) * sourceYplus1[(x+1)] + TS(5) * sourceYplus1[(x+2)] + TS(1) * sourceYplus1[(x+3)];
                
                TS sum3 = TS(1) * sourceYminus1[(x+2)] + TS(5) * sourceYminus1[(x+3)] + TS(1) * sourceYminus1[(x+4)];
                sum3 += TS(5) * sourceYplus0[(x+2)] + TS(8) * sourceYplus0[(x+3)] + TS(5) * sourceYplus0[(x+4)];
                sum3 += TS(1) * sourceYplus1[(x+2)] + TS(5) * sourceYplus1[(x+3)] + TS(1) * sourceYplus1[(x+4)];
                
                
                TS sum4 = TS(1) * sourceYminus1[(x+3)] + TS(5) * sourceYminus1[(x+4)] + TS(1) * sourceYminus1[(x+5)];
                sum4 += TS(5) * sourceYplus0[(x+3)] + TS(8) * sourceYplus0[(x+4)] + TS(5) * sourceYplus0[(x+5)];
                sum4 += TS(1) * sourceYplus1[(x+3)] + TS(5) * sourceYplus1[(x+4)] + TS(1) * sourceYplus1[(x+5)];
                
                TS sum5 = TS(1) * sourceYminus1[(x+4)] + TS(5) * sourceYminus1[(x+5)] + TS(1) * sourceYminus1[(x+6)];
                sum5 += TS(5) * sourceYplus0[(x+4)] + TS(8) * sourceYplus0[(x+5)] + TS(5) * sourceYplus0[(x+6)];
                sum5 += TS(1) * sourceYplus1[(x+4)] + TS(5) * sourceYplus1[(x+5)] + TS(1) * sourceYplus1[(x+6)];
                
                TS sum6 = TS(1) * sourceYminus1[(x+5)] + TS(5) * sourceYminus1[(x+6)] + TS(1) * sourceYminus1[(x+7)];
                sum6 += TS(5) * sourceYplus0[(x+5)] + TS(8) * sourceYplus0[(x+6)] + TS(5) * sourceYplus0[(x+7)];
                sum6 += TS(1) * sourceYplus1[(x+5)] + TS(5) * sourceYplus1[(x+6)] + TS(1) * sourceYplus1[(x+7)];
                
                TS sum7 = TS(1) * sourceYminus1[(x+6)] + TS(5) * sourceYminus1[(x+7)] + TS(1) * sourceYminus1[(x+8)];
                sum7 += TS(5) * sourceYplus0[(x+6)] + TS(8) * sourceYplus0[(x+7)] + TS(5) * sourceYplus0[(x+8)];
                sum7 += TS(1) * sourceYplus1[(x+6)] + TS(5) * sourceYplus1[(x+7)] + TS(1) * sourceYplus1[(x+8)];
                
                T temp0 = T( (sum0+half) / T(32) );
                T temp1 = T( (sum1+half) / T(32) );
                T temp2 = T( (sum2+half) / T(32) );
                T temp3 = T( (sum3+half) / T(32) );
                T temp4 = T( (sum4+half) / T(32) );
                T temp5 = T( (sum5+half) / T(32) );
                T temp6 = T( (sum6+half) / T(32) );
                T temp7 = T( (sum7+half) / T(32) );
                
                dest2[x+0] = temp0;
                dest2[x+1] = temp1;
                dest2[x+2] = temp2;
                dest2[x+3] = temp3;
                dest2[x+4] = temp4;
                dest2[x+5] = temp5;
                dest2[x+6] = temp6;
                dest2[x+7] = temp7;
            }

            for ( ; x < (cols-1); ++x ) {
                TS sum = TS(1) * sourceYminus1[(x-1)] + TS(5) * sourceYminus1[(x+0)] + TS(1) * sourceYminus1[(x+1)];
                sum += TS(5) * sourceYplus0[(x-1)] + TS(8) * sourceYplus0[(x+0)] + TS(5) * sourceYplus0[(x+1)];
                sum += TS(1) * sourceYplus1[(x-1)] + TS(5) * sourceYplus1[(x+0)] + TS(1) * sourceYplus1[(x+1)];
                T temp = T( (sum+half) / T(32) );
                dest2[x+0] = temp;
            }
            
            sourceYminus1 += rowStep;
            sourceYplus0 += rowStep;
            sourceYplus1 += rowStep;
            dest2 += rowStep;
        }

    }
    check_add_2D(1,dest,rows,cols,rowStep,label);
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

// move loop invariant values out
// unroll 4x
// rotate the values so they are only read once
template <typename T, typename TS >
void convolution2D_opt6(const T *source, T *dest, int rows, int cols, int rowStep, const std::string &label) {
    int i;
    const bool isFloat = (T(2.9) > T(2.0));
    const TS half = isFloat ? TS(0) : TS(16);

    start_timer();

    for(i = 0; i < iterations; ++i) {
        int x, y;
        
        const T *sourceYminus1 = &source[((1-1)*rowStep)];
        const T *sourceYplus0 = &source[((1+0)*rowStep)];
        const T *sourceYplus1 = &source[((1+1)*rowStep)];
        T *dest2 = dest + rowStep;
        
        for (y = 1; y < (rows - 1); ++ y) {
            T sourceYminus1Xminus1 = sourceYminus1[(1-1)];
            T sourceYminus1Xplus0 = sourceYminus1[(1+0)];
            T sourceYplus0Xminus1 = sourceYplus0[(1-1)];
            T sourceYplus0Xplus0 = sourceYplus0[(1+0)];
            T sourceYplus1Xminus1 = sourceYplus1[(1-1)];
            T sourceYplus1Xplus0 = sourceYplus1[(1+0)];

            for (x = 1; x < ((cols-1)-3); x += 4) {
                T sourceYminus1Xplus1 = sourceYminus1[(x+1)];
                T sourceYplus0Xplus1 = sourceYplus0[(x+1)];
                T sourceYplus1Xplus1 = sourceYplus1[(x+1)];
                T sourceYminus1Xplus2 = sourceYminus1[(x+2)];
                T sourceYplus0Xplus2 = sourceYplus0[(x+2)];
                T sourceYplus1Xplus2 = sourceYplus1[(x+2)];
                T sourceYminus1Xplus3 = sourceYminus1[(x+3)];
                T sourceYplus0Xplus3 = sourceYplus0[(x+3)];
                T sourceYplus1Xplus3 = sourceYplus1[(x+3)];
                T sourceYminus1Xplus4 = sourceYminus1[(x+4)];
                T sourceYplus0Xplus4 = sourceYplus0[(x+4)];
                T sourceYplus1Xplus4 = sourceYplus1[(x+4)];
                
                TS sum0 = TS(1) * sourceYminus1Xminus1 + TS(5) * sourceYminus1Xplus0 + TS(1) * sourceYminus1Xplus1;
                sum0 += TS(5) * sourceYplus0Xminus1 + TS(8) * sourceYplus0Xplus0 + TS(5) * sourceYplus0Xplus1;
                sum0 += TS(1) * sourceYplus1Xminus1 + TS(5) * sourceYplus1Xplus0 + TS(1) * sourceYplus1Xplus1;
                
                TS sum1 = TS(1) * sourceYminus1Xplus0 + TS(5) * sourceYminus1Xplus1 + TS(1) * sourceYminus1Xplus2;
                sum1 += TS(5) * sourceYplus0Xplus0 + TS(8) * sourceYplus0Xplus1 + TS(5) * sourceYplus0Xplus2;
                sum1 += TS(1) * sourceYplus1Xplus0 + TS(5) * sourceYplus1Xplus1 + TS(1) * sourceYplus1Xplus2;
                
                TS sum2 = TS(1) * sourceYminus1Xplus1 + TS(5) * sourceYminus1Xplus2 + TS(1) * sourceYminus1Xplus3;
                sum2 += TS(5) * sourceYplus0Xplus1 + TS(8) * sourceYplus0Xplus2 + TS(5) * sourceYplus0Xplus3;
                sum2 += TS(1) * sourceYplus1Xplus1 + TS(5) * sourceYplus1Xplus2 + TS(1) * sourceYplus1Xplus3;
                
                TS sum3 = TS(1) * sourceYminus1Xplus2 + TS(5) * sourceYminus1Xplus3 + TS(1) * sourceYminus1Xplus4;
                sum3 += TS(5) * sourceYplus0Xplus2 + TS(8) * sourceYplus0Xplus3 + TS(5) * sourceYplus0Xplus4;
                sum3 += TS(1) * sourceYplus1Xplus2 + TS(5) * sourceYplus1Xplus3 + TS(1) * sourceYplus1Xplus4;
                
                T temp0 = T( (sum0+half) / T(32) );
                T temp1 = T( (sum1+half) / T(32) );
                T temp2 = T( (sum2+half) / T(32) );
                T temp3 = T( (sum3+half) / T(32) );
                
                sourceYminus1Xminus1 = sourceYminus1Xplus3;
                sourceYminus1Xplus0 = sourceYminus1Xplus4;
                sourceYplus0Xminus1 = sourceYplus0Xplus3;
                sourceYplus0Xplus0 = sourceYplus0Xplus4;
                sourceYplus1Xminus1 = sourceYplus1Xplus3;
                sourceYplus1Xplus0 = sourceYplus1Xplus4;
                
                dest2[x+0] = temp0;
                dest2[x+1] = temp1;
                dest2[x+2] = temp2;
                dest2[x+3] = temp3;
            }

            for ( ; x < (cols-1); ++x ) {
                T sourceYminus1Xplus1 = sourceYminus1[(x+1)];
                T sourceYplus0Xplus1 = sourceYplus0[(x+1)];
                T sourceYplus1Xplus1 = sourceYplus1[(x+1)];
                TS sum = TS(1) * sourceYminus1Xminus1 + TS(5) * sourceYminus1Xplus0 + TS(1) * sourceYminus1Xplus1;
                sum += TS(5) * sourceYplus0Xminus1 + TS(8) * sourceYplus0Xplus0 + TS(5) * sourceYplus0Xplus1;
                sum += TS(1) * sourceYplus1Xminus1 + TS(5) * sourceYplus1Xplus0 + TS(1) * sourceYplus1Xplus1;
                T temp = T( (sum+half) / T(32) );
                sourceYminus1Xminus1 = sourceYminus1Xplus0;
                sourceYminus1Xplus0 = sourceYminus1Xplus1;
                sourceYplus0Xminus1 = sourceYplus0Xplus0;
                sourceYplus0Xplus0 = sourceYplus0Xplus1;
                sourceYplus1Xminus1 = sourceYplus1Xplus0;
                sourceYplus1Xplus0 = sourceYplus1Xplus1;
                dest2[x+0] = temp;
            }
            
            sourceYminus1 += rowStep;
            sourceYplus0 += rowStep;
            sourceYplus1 += rowStep;
            dest2 += rowStep;
        }
    }
    check_add_2D(1,dest,rows,cols,rowStep,label);
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

// move loop invariant values out
// unroll 4x
// rotate the values so they are only read once
// collect common terms (subexpressions)
template <typename T, typename TS >
void convolution2D_opt7(const T *source, T *dest, int rows, int cols, int rowStep, const std::string &label) {
    int i;
    const bool isFloat = (T(2.9) > T(2.0));
    const TS half = isFloat ? TS(0) : TS(16);

    start_timer();

    for(i = 0; i < iterations; ++i) {
        int x, y;
        
        const T *sourceYminus1 = &source[((1-1)*rowStep)];
        const T *sourceYplus0 = &source[((1+0)*rowStep)];
        const T *sourceYplus1 = &source[((1+1)*rowStep)];
        T *dest2 = dest + rowStep;
        
        for (y = 1; y < (rows - 1); ++ y) {
            T sourceYminus1Xminus1 = sourceYminus1[(1-1)];
            T sourceYminus1Xplus0 = sourceYminus1[(1+0)];
            T sourceYplus0Xminus1 = sourceYplus0[(1-1)];
            T sourceYplus0Xplus0 = sourceYplus0[(1+0)];
            T sourceYplus1Xminus1 = sourceYplus1[(1-1)];
            T sourceYplus1Xplus0 = sourceYplus1[(1+0)];

            for (x = 1; x < (cols-1-3); x += 4) {
                T sourceYminus1Xplus1 = sourceYminus1[(x+1)];
                T sourceYplus0Xplus1 = sourceYplus0[(x+1)];
                T sourceYplus1Xplus1 = sourceYplus1[(x+1)];
                T sourceYminus1Xplus2 = sourceYminus1[(x+2)];
                T sourceYplus0Xplus2 = sourceYplus0[(x+2)];
                T sourceYplus1Xplus2 = sourceYplus1[(x+2)];
                T sourceYminus1Xplus3 = sourceYminus1[(x+3)];
                T sourceYplus0Xplus3 = sourceYplus0[(x+3)];
                T sourceYplus1Xplus3 = sourceYplus1[(x+3)];
                T sourceYminus1Xplus4 = sourceYminus1[(x+4)];
                T sourceYplus0Xplus4 = sourceYplus0[(x+4)];
                T sourceYplus1Xplus4 = sourceYplus1[(x+4)];
                
                TS tempCol1 = TS(1) * sourceYminus1Xplus1 + TS(5) * sourceYplus0Xplus1 + TS(1) * sourceYplus1Xplus1;
                TS tempCol2 = TS(1) * sourceYminus1Xplus2 + TS(5) * sourceYplus0Xplus2 + TS(1) * sourceYplus1Xplus2;
                
                TS sum0 = TS(1) * sourceYminus1Xminus1 + TS(5) * sourceYminus1Xplus0 + tempCol1;
                sum0 += TS(5) * sourceYplus0Xminus1 + TS(8) * sourceYplus0Xplus0;
                sum0 += TS(1) * sourceYplus1Xminus1 + TS(5) * sourceYplus1Xplus0;
                
                TS sum1 = TS(1) * sourceYminus1Xplus0 + TS(5) * sourceYminus1Xplus1 + tempCol2;
                sum1 += TS(5) * sourceYplus0Xplus0 + TS(8) * sourceYplus0Xplus1;
                sum1 += TS(1) * sourceYplus1Xplus0 + TS(5) * sourceYplus1Xplus1;
                
                TS sum2 = TS(5) * sourceYminus1Xplus2 + TS(1) * sourceYminus1Xplus3 + tempCol1;
                sum2 += TS(8) * sourceYplus0Xplus2 + TS(5) * sourceYplus0Xplus3;
                sum2 += TS(5) * sourceYplus1Xplus2 + TS(1) * sourceYplus1Xplus3;
                
                TS sum3 = TS(5) * sourceYminus1Xplus3 + TS(1) * sourceYminus1Xplus4 + tempCol2;
                sum3 += TS(8) * sourceYplus0Xplus3 + TS(5) * sourceYplus0Xplus4;
                sum3 += TS(5) * sourceYplus1Xplus3 + TS(1) * sourceYplus1Xplus4;
                
                T temp0 = T( (sum0+half) / T(32) );
                T temp1 = T( (sum1+half) / T(32) );
                T temp2 = T( (sum2+half) / T(32) );
                T temp3 = T( (sum3+half) / T(32) );
                
                sourceYminus1Xminus1 = sourceYminus1Xplus3;
                sourceYminus1Xplus0 = sourceYminus1Xplus4;
                sourceYplus0Xminus1 = sourceYplus0Xplus3;
                sourceYplus0Xplus0 = sourceYplus0Xplus4;
                sourceYplus1Xminus1 = sourceYplus1Xplus3;
                sourceYplus1Xplus0 = sourceYplus1Xplus4;
                
                dest2[x+0] = temp0;
                dest2[x+1] = temp1;
                dest2[x+2] = temp2;
                dest2[x+3] = temp3;
            }

            for ( ; x < (cols-1); ++x ) {
                T sourceYminus1Xplus1 = sourceYminus1[(x+1)];
                T sourceYplus0Xplus1 = sourceYplus0[(x+1)];
                T sourceYplus1Xplus1 = sourceYplus1[(x+1)];
                TS sum = TS(1) * sourceYminus1Xminus1 + TS(5) * sourceYminus1Xplus0 + TS(1) * sourceYminus1Xplus1;
                sum += TS(5) * sourceYplus0Xminus1 + TS(8) * sourceYplus0Xplus0 + TS(5) * sourceYplus0Xplus1;
                sum += TS(1) * sourceYplus1Xminus1 + TS(5) * sourceYplus1Xplus0 + TS(1) * sourceYplus1Xplus1;
                T temp = T( (sum+half) / T(32) );
                sourceYminus1Xminus1 = sourceYminus1Xplus0;
                sourceYminus1Xplus0 = sourceYminus1Xplus1;
                sourceYplus0Xminus1 = sourceYplus0Xplus0;
                sourceYplus0Xplus0 = sourceYplus0Xplus1;
                sourceYplus1Xminus1 = sourceYplus1Xplus0;
                sourceYplus1Xplus0 = sourceYplus1Xplus1;
                dest2[x+0] = temp;
            }
            
            sourceYminus1 += rowStep;
            sourceYplus0 += rowStep;
            sourceYplus1 += rowStep;
            dest2 += rowStep;
        }
    }
    check_add_2D(1,dest,rows,cols,rowStep,label);
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

// move loop invariant values out
// unroll 8x
// give the compiler some CSE hints
template <typename T, typename TS >
void convolution2D_opt8(const T *source, T *dest, int rows, int cols, int rowStep, const std::string &label) {
    int i;
    const bool isFloat = (T(2.9) > T(2.0));
    const TS half = isFloat ? TS(0) : TS(16);

    start_timer();

    for(i = 0; i < iterations; ++i) {
        int x, y;
        
        const T *sourceYminus1 = &source[((1-1)*rowStep)];
        const T *sourceYplus0 = &source[((1+0)*rowStep)];
        const T *sourceYplus1 = &source[((1+1)*rowStep)];
        T *dest2 = dest + rowStep;
        
        for (y = 1; y < (rows - 1); ++ y) {

            for (x = 1; x < ((cols-1)-7); x += 8) {
                TS sum0 = (TS(1) * sourceYminus1[(x-1)] + TS(5) * sourceYplus0[(x-1)] + TS(1) * sourceYplus1[(x-1)]);
                sum0 += (TS(1) * sourceYminus1[(x+1)] + TS(5) * sourceYplus0[(x+1)] + TS(1) * sourceYplus1[(x+1)]);
                sum0 += TS(5) * sourceYminus1[(x+0)] + TS(8) * sourceYplus0[(x+0)] + TS(5) * sourceYplus1[(x+0)];
                
                TS sum1 = (TS(1) * sourceYminus1[(x+0)] + TS(5) * sourceYplus0[(x+0)] + TS(1) * sourceYplus1[(x+0)]);
                sum1 += (TS(1) * sourceYminus1[(x+2)] + TS(5) * sourceYplus0[(x+2)] + TS(1) * sourceYplus1[(x+2)]);
                sum1 += TS(5) * sourceYminus1[(x+1)] + TS(8) * sourceYplus0[(x+1)] + TS(5) * sourceYplus1[(x+1)];
                
                TS sum2 = (TS(1) * sourceYminus1[(x+1)] + TS(5) * sourceYplus0[(x+1)] + TS(1) * sourceYplus1[(x+1)]);
                sum2 += (TS(1) * sourceYminus1[(x+3)] + TS(5) * sourceYplus0[(x+3)] + TS(1) * sourceYplus1[(x+3)]);
                sum2 += TS(5) * sourceYminus1[(x+2)] + TS(8) * sourceYplus0[(x+2)] + TS(5) * sourceYplus1[(x+2)];
                
                TS sum3 = (TS(1) * sourceYminus1[(x+2)] + TS(5) * sourceYplus0[(x+2)] + TS(1) * sourceYplus1[(x+2)]);
                sum3 += (TS(1) * sourceYminus1[(x+4)] + TS(5) * sourceYplus0[(x+4)] + TS(1) * sourceYplus1[(x+4)]);
                sum3 += TS(5) * sourceYminus1[(x+3)] + TS(8) * sourceYplus0[(x+3)] + TS(5) * sourceYplus1[(x+3)];
                
                TS sum4 = (TS(1) * sourceYminus1[(x+3)] + TS(5) * sourceYplus0[(x+3)] + TS(1) * sourceYplus1[(x+3)]);
                sum4 += (TS(1) * sourceYminus1[(x+5)] + TS(5) * sourceYplus0[(x+5)] + TS(1) * sourceYplus1[(x+5)]);
                sum4 += TS(5) * sourceYminus1[(x+4)] + TS(8) * sourceYplus0[(x+4)] + TS(5) * sourceYplus1[(x+4)];
                
                TS sum5 = (TS(1) * sourceYminus1[(x+4)] + TS(5) * sourceYplus0[(x+4)] + TS(1) * sourceYplus1[(x+4)]);
                sum5 += (TS(1) * sourceYminus1[(x+6)] + TS(5) * sourceYplus0[(x+6)] + TS(1) * sourceYplus1[(x+6)]);
                sum5 += TS(5) * sourceYminus1[(x+5)] + TS(8) * sourceYplus0[(x+5)] + TS(5) * sourceYplus1[(x+5)];
                
                TS sum6 = (TS(1) * sourceYminus1[(x+5)] + TS(5) * sourceYplus0[(x+5)] + TS(1) * sourceYplus1[(x+5)]);
                sum6 += (TS(1) * sourceYminus1[(x+7)] + TS(5) * sourceYplus0[(x+7)] + TS(1) * sourceYplus1[(x+7)]);
                sum6 += TS(5) * sourceYminus1[(x+6)] + TS(8) * sourceYplus0[(x+6)] + TS(5) * sourceYplus1[(x+6)];
                
                TS sum7 = (TS(1) * sourceYminus1[(x+6)] + TS(5) * sourceYplus0[(x+6)] + TS(1) * sourceYplus1[(x+6)]);
                sum7 += (TS(1) * sourceYminus1[(x+8)] + TS(5) * sourceYplus0[(x+8)] + TS(1) * sourceYplus1[(x+8)]);
                sum7 += TS(5) * sourceYminus1[(x+7)] + TS(8) * sourceYplus0[(x+7)] + TS(5) * sourceYplus1[(x+7)];
                
                T temp0 = T( (sum0+half) / T(32) );
                T temp1 = T( (sum1+half) / T(32) );
                T temp2 = T( (sum2+half) / T(32) );
                T temp3 = T( (sum3+half) / T(32) );
                T temp4 = T( (sum4+half) / T(32) );
                T temp5 = T( (sum5+half) / T(32) );
                T temp6 = T( (sum6+half) / T(32) );
                T temp7 = T( (sum7+half) / T(32) );
                
                dest2[x+0] = temp0;
                dest2[x+1] = temp1;
                dest2[x+2] = temp2;
                dest2[x+3] = temp3;
                dest2[x+4] = temp4;
                dest2[x+5] = temp5;
                dest2[x+6] = temp6;
                dest2[x+7] = temp7;
            }

            for ( ; x < (cols-1); ++x ) {
                TS sum0 = (TS(1) * sourceYminus1[(x-1)] + TS(5) * sourceYplus0[(x-1)] + TS(1) * sourceYplus1[(x-1)]);
                sum0 += (TS(1) * sourceYminus1[(x+1)] + TS(5) * sourceYplus0[(x+1)] + TS(1) * sourceYplus1[(x+1)]);
                sum0 += TS(5) * sourceYminus1[(x+0)] + TS(8) * sourceYplus0[(x+0)] + TS(5) * sourceYplus1[(x+0)];
                T temp0 = T( (sum0+half) / T(32) );
                dest2[x+0] = temp0;
            }
            
            sourceYminus1 += rowStep;
            sourceYplus0 += rowStep;
            sourceYplus1 += rowStep;
            dest2 += rowStep;
        }
    }
    check_add_2D(1,dest,rows,cols,rowStep,label);
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

// move loop invariant values out
// unroll 8x
// collect common terms (subexpressions)
template <typename T, typename TS >
void convolution2D_opt9(const T *source, T *dest, int rows, int cols, int rowStep, const std::string &label) {
    int i;
    const bool isFloat = (T(2.9) > T(2.0));
    const TS half = isFloat ? TS(0) : TS(16);

    start_timer();

    for(i = 0; i < iterations; ++i) {
        int x, y;
        
        const T *sourceYminus1 = &source[((1-1)*rowStep)];
        const T *sourceYplus0 = &source[((1+0)*rowStep)];
        const T *sourceYplus1 = &source[((1+1)*rowStep)];
        T *dest2 = dest + rowStep;
        
        for (y = 1; y < (rows - 1); ++ y) {

            T sourceYminus1Xminus1 = sourceYminus1[(1-1)];
            T sourceYplus0Xminus1 = sourceYplus0[(1-1)];
            T sourceYplus1Xminus1 = sourceYplus1[(1-1)];
            
            TS tempColN1 = TS(1) * sourceYminus1Xminus1 + TS(5) * sourceYplus0Xminus1 + TS(1) * sourceYplus1Xminus1;
            
            T sourceYminus1Xplus0 = sourceYminus1[(1+0)];
            T sourceYplus0Xplus0 = sourceYplus0[(1+0)];
            T sourceYplus1Xplus0 = sourceYplus1[(1+0)];
            
            TS tempCol0 = TS(1) * sourceYminus1Xplus0 + TS(5) * sourceYplus0Xplus0 + TS(1) * sourceYplus1Xplus0;

            for (x = 1; x < (cols-1-7); x += 8) {
                
                T sourceYminus1Xplus1 = sourceYminus1[(x+1)];
                T sourceYplus0Xplus1 = sourceYplus0[(x+1)];
                T sourceYplus1Xplus1 = sourceYplus1[(x+1)];
                
                T sourceYminus1Xplus2 = sourceYminus1[(x+2)];
                T sourceYplus0Xplus2 = sourceYplus0[(x+2)];
                T sourceYplus1Xplus2 = sourceYplus1[(x+2)];
                
                T sourceYminus1Xplus3 = sourceYminus1[(x+3)];
                T sourceYplus0Xplus3 = sourceYplus0[(x+3)];
                T sourceYplus1Xplus3 = sourceYplus1[(x+3)];
                
                T sourceYminus1Xplus4 = sourceYminus1[(x+4)];
                T sourceYplus0Xplus4 = sourceYplus0[(x+4)];
                T sourceYplus1Xplus4 = sourceYplus1[(x+4)];
                
                T sourceYminus1Xplus5 = sourceYminus1[(x+5)];
                T sourceYplus0Xplus5 = sourceYplus0[(x+5)];
                T sourceYplus1Xplus5 = sourceYplus1[(x+5)];
                
                T sourceYminus1Xplus6 = sourceYminus1[(x+6)];
                T sourceYplus0Xplus6 = sourceYplus0[(x+6)];
                T sourceYplus1Xplus6 = sourceYplus1[(x+6)];
                
                T sourceYminus1Xplus7 = sourceYminus1[(x+7)];
                T sourceYplus0Xplus7 = sourceYplus0[(x+7)];
                T sourceYplus1Xplus7 = sourceYplus1[(x+7)];
                
                T sourceYminus1Xplus8 = sourceYminus1[(x+8)];
                T sourceYplus0Xplus8 = sourceYplus0[(x+8)];
                T sourceYplus1Xplus8 = sourceYplus1[(x+8)];
                
                TS tempCol1 = TS(1) * sourceYminus1Xplus1 + TS(5) * sourceYplus0Xplus1 + TS(1) * sourceYplus1Xplus1;
                TS tempCol2 = TS(1) * sourceYminus1Xplus2 + TS(5) * sourceYplus0Xplus2 + TS(1) * sourceYplus1Xplus2;
                TS tempCol3 = TS(1) * sourceYminus1Xplus3 + TS(5) * sourceYplus0Xplus3 + TS(1) * sourceYplus1Xplus3;
                TS tempCol4 = TS(1) * sourceYminus1Xplus4 + TS(5) * sourceYplus0Xplus4 + TS(1) * sourceYplus1Xplus4;
                TS tempCol5 = TS(1) * sourceYminus1Xplus5 + TS(5) * sourceYplus0Xplus5 + TS(1) * sourceYplus1Xplus5;
                TS tempCol6 = TS(1) * sourceYminus1Xplus6 + TS(5) * sourceYplus0Xplus6 + TS(1) * sourceYplus1Xplus6;
                TS tempCol7 = TS(1) * sourceYminus1Xplus7 + TS(5) * sourceYplus0Xplus7 + TS(1) * sourceYplus1Xplus7;
                TS tempCol8 = TS(1) * sourceYminus1Xplus8 + TS(5) * sourceYplus0Xplus8 + TS(1) * sourceYplus1Xplus8;
                
                TS sum0 = TS(5) * sourceYminus1Xplus0 + tempColN1 + tempCol1;
                sum0 += TS(8) * sourceYplus0Xplus0;
                sum0 += TS(5) * sourceYplus1Xplus0;
                
                TS sum1 = TS(5) * sourceYminus1Xplus1 + tempCol0 + tempCol2;
                sum1 += TS(8) * sourceYplus0Xplus1;
                sum1 += TS(5) * sourceYplus1Xplus1;
                
                TS sum2 = TS(5) * sourceYminus1Xplus2 + tempCol3 + tempCol1;
                sum2 += TS(8) * sourceYplus0Xplus2;
                sum2 += TS(5) * sourceYplus1Xplus2;
                
                TS sum3 = TS(5) * sourceYminus1Xplus3 + tempCol2 + tempCol4;
                sum3 += TS(8) * sourceYplus0Xplus3;
                sum3 += TS(5) * sourceYplus1Xplus3;
                
                TS sum4 = TS(5) * sourceYminus1Xplus4 + tempCol5 + tempCol3;
                sum4 += TS(8) * sourceYplus0Xplus4;
                sum4 += TS(5) * sourceYplus1Xplus4;
                
                TS sum5 = TS(5) * sourceYminus1[(x+5)] + tempCol6 + tempCol4;
                sum5 += TS(8) * sourceYplus0[(x+5)];
                sum5 += TS(5) * sourceYplus1[(x+5)];
                
                TS sum6 = TS(5) * sourceYminus1[(x+6)] + tempCol5 + tempCol7;
                sum6 += TS(8) * sourceYplus0[(x+6)];
                sum6 += TS(5) * sourceYplus1[(x+6)];
                
                TS sum7 = TS(5) * sourceYminus1[(x+7)] + tempCol6 + tempCol8;
                sum7 += TS(8) * sourceYplus0[(x+7)];
                sum7 += TS(5) * sourceYplus1[(x+7)];
                
                tempColN1 = tempCol7;
                tempCol0 = tempCol8;
                
                T temp0 = T( (sum0+half) / T(32) );
                T temp1 = T( (sum1+half) / T(32) );
                T temp2 = T( (sum2+half) / T(32) );
                T temp3 = T( (sum3+half) / T(32) );
                T temp4 = T( (sum4+half) / T(32) );
                T temp5 = T( (sum5+half) / T(32) );
                T temp6 = T( (sum6+half) / T(32) );
                T temp7 = T( (sum7+half) / T(32) );
                
                dest2[x+0] = temp0;
                dest2[x+1] = temp1;
                dest2[x+2] = temp2;
                dest2[x+3] = temp3;
                dest2[x+4] = temp4;
                dest2[x+5] = temp5;
                dest2[x+6] = temp6;
                dest2[x+7] = temp7;
            }

            for ( ; x < (cols-1); ++x ) {
                
                T sourceYminus1Xplus0 = sourceYminus1[(x+0)];
                T sourceYplus0Xplus0 = sourceYplus0[(x+0)];
                T sourceYplus1Xplus0 = sourceYplus1[(x+0)];
                
                T sourceYminus1Xplus1 = sourceYminus1[(x+1)];
                T sourceYplus0Xplus1 = sourceYplus0[(x+1)];
                T sourceYplus1Xplus1 = sourceYplus1[(x+1)];
                
                TS tempCol1 = TS(1) * sourceYminus1Xplus1 + TS(5) * sourceYplus0Xplus1 + TS(1) * sourceYplus1Xplus1;
                
                TS sum = TS(5) * sourceYminus1Xplus0 + tempColN1 + tempCol1;
                sum += TS(8) * sourceYplus0Xplus0;
                sum += TS(5) * sourceYplus1Xplus0;
                
                tempColN1 = tempCol0;
                tempCol0 = tempCol1;
                
                T temp = T( (sum+half) / T(32) );
                dest2[x+0] = temp;
            }
            
            sourceYminus1 += rowStep;
            sourceYplus0 += rowStep;
            sourceYplus1 += rowStep;
            dest2 += rowStep;
        }

    }
    check_add_2D(1,dest,rows,cols,rowStep,label);
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/
/******************************************************************************/

/*
    2D convolution using a separable filter (gaussian, sort of)
    hard coded filter
    ignoring edges
 
    1 3 8 3 1
    result divided by 16
*/
template <typename T, typename TS >
void convolution2D_sep(const T *source, T *dest, int rows, int cols, int rowStep, const std::string &label) {
    int i;
    const bool isFloat = (T(2.9) > T(2.0));
    const TS half = isFloat ? TS(0) : TS(8);

    start_timer();

    for( i = 0; i < iterations; ++i ) {
        int x, y;

        // horizontal pass
        for (y = 0; y < rows; ++y) {
            for (x = 2; x < (cols-2); ++x) {
                TS sum = TS(1) * source[(y*rowStep)+x-2] + TS(3) * source[(y*rowStep)+x-1] + TS(8) * source[(y*rowStep)+x+0] + TS(3) * source[(y*rowStep)+x+1] + TS(1) * source[(y*rowStep)+x+2];
                T temp = T( (sum+half) / TS(16) );
                dest[(y*rowStep + x)] = temp;
            }
        }

        // vertical pass
        for (x = 2; x < (cols-2); ++x) {
            for (y = 2; y < (rows-2); ++y) {
                TS sum = TS(1) * dest[(y-2)*rowStep+x] + TS(3) * dest[(y-1)*rowStep+x] + TS(8) * dest[(y+0)*rowStep+x] + TS(3) * dest[(y+1)*rowStep+x] + TS(1) * dest[(y+2)*rowStep+x];
                T temp = T( (sum+half) / TS(16) );
                dest[(y*rowStep + x)] = temp;
            }
        }
    }
    check_add_2D(2,dest,rows,cols,rowStep,label);
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}


/******************************************************************************/

// swap loops
template <typename T, typename TS >
void convolution2D_sep_swap(const T *source, T *dest, int rows, int cols, int rowStep, const std::string &label) {
    int i;
    const bool isFloat = (T(2.9) > T(2.0));
    const TS half = isFloat ? TS(0) : TS(8);

    start_timer();

    for( i = 0; i < iterations; ++i ) {
        int x, y;

        // horizontal pass
        for (x = 2; x < (cols-2); ++x) {
            for (y = 0; y < rows; ++y) {
                TS sum = TS(1) * source[(y*rowStep)+x-2] + TS(3) * source[(y*rowStep)+x-1] + TS(8) * source[(y*rowStep)+x+0] + TS(3) * source[(y*rowStep)+x+1] + TS(1) * source[(y*rowStep)+x+2];
                T temp = T( (sum+half) / TS(16) );
                dest[(y*rowStep + x)] = temp;
            }
        }

        // vertical pass
        for (y = 2; y < (rows-2); ++y) {
            for (x = 2; x < (cols-2); ++x) {
                TS sum = TS(1) * dest[(y-2)*rowStep+x] + TS(3) * dest[(y-1)*rowStep+x] + TS(8) * dest[(y+0)*rowStep+x] + TS(3) * dest[(y+1)*rowStep+x] + TS(1) * dest[(y+2)*rowStep+x];
                T temp = T( (sum+half) / TS(16) );
                dest[(y*rowStep + x)] = temp;
            }
        }
    }
    check_add_2D(2,dest,rows,cols,rowStep,label);
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}


/******************************************************************************/

// swap only the horizontal loop
template <typename T, typename TS >
void convolution2D_sep_swaphoriz(const T *source, T *dest, int rows, int cols, int rowStep, const std::string &label) {
    int i;
    const bool isFloat = (T(2.9) > T(2.0));
    const TS half = isFloat ? TS(0) : TS(8);

    start_timer();

    for( i = 0; i < iterations; ++i ) {
        int x, y;

        // horizontal pass
        for (x = 2; x < (cols-2); ++x) {
            for (y = 0; y < rows; ++y) {
                TS sum = TS(1) * source[(y*rowStep)+x-2] + TS(3) * source[(y*rowStep)+x-1] + TS(8) * source[(y*rowStep)+x+0] + TS(3) * source[(y*rowStep)+x+1] + TS(1) * source[(y*rowStep)+x+2];
                T temp = T( (sum+half) / TS(16) );
                dest[(y*rowStep + x)] = temp;
            }
        }

        // vertical pass
        for (x = 2; x < (cols-2); ++x) {
            for (y = 2; y < (rows-2); ++y) {
                TS sum = TS(1) * dest[(y-2)*rowStep+x] + TS(3) * dest[(y-1)*rowStep+x] + TS(8) * dest[(y+0)*rowStep+x] + TS(3) * dest[(y+1)*rowStep+x] + TS(1) * dest[(y+2)*rowStep+x];
                T temp = T( (sum+half) / TS(16) );
                dest[(y*rowStep + x)] = temp;
            }
        }
    }
    check_add_2D(2,dest,rows,cols,rowStep,label);
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}


/******************************************************************************/

// swap only the vertical loop
template <typename T, typename TS >
void convolution2D_sep_swapvert(const T *source, T *dest, int rows, int cols, int rowStep, const std::string &label) {
    int i;
    const bool isFloat = (T(2.9) > T(2.0));
    const TS half = isFloat ? TS(0) : TS(8);

    start_timer();

    for( i = 0; i < iterations; ++i ) {
        int x, y;

        // horizontal pass
        for (y = 0; y < rows; ++y) {
            for (x = 2; x < (cols-2); ++x) {
                TS sum = TS(1) * source[(y*rowStep)+x-2] + TS(3) * source[(y*rowStep)+x-1] + TS(8) * source[(y*rowStep)+x+0] + TS(3) * source[(y*rowStep)+x+1] + TS(1) * source[(y*rowStep)+x+2];
                T temp = T( (sum+half) / TS(16) );
                dest[(y*rowStep + x)] = temp;
            }
        }

        // vertical pass
        for (y = 2; y < (rows-2); ++y) {
            for (x = 2; x < (cols-2); ++x) {
                TS sum = TS(1) * dest[(y-2)*rowStep+x] + TS(3) * dest[(y-1)*rowStep+x] + TS(8) * dest[(y+0)*rowStep+x] + TS(3) * dest[(y+1)*rowStep+x] + TS(1) * dest[(y+2)*rowStep+x];
                T temp = T( (sum+half) / TS(16) );
                dest[(y*rowStep + x)] = temp;
            }
        }
    }
    check_add_2D(2,dest,rows,cols,rowStep,label);
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

// reverse loops
template <typename T, typename TS >
void convolution2D_sep_reverse(const T *source, T *dest, int rows, int cols, int rowStep, const std::string &label) {
    int i;
    const bool isFloat = (T(2.9) > T(2.0));
    const TS half = isFloat ? TS(0) : TS(8);

    start_timer();

    for( i = 0; i < iterations; ++i ) {
        int x, y;

        // horizontal pass
        for (y = rows-1; y >= 0; --y) {
            for (x = (cols-3); x > 1 ; --x) {
                TS sum = TS(1) * source[(y*rowStep)+x-2] + TS(3) * source[(y*rowStep)+x-1] + TS(8) * source[(y*rowStep)+x+0] + TS(3) * source[(y*rowStep)+x+1] + TS(1) * source[(y*rowStep)+x+2];
                T temp = T( (sum+half) / TS(16) );
                dest[(y*rowStep + x)] = temp;
            }
        }

        // vertical pass
        for (x = (cols-3); x > 1 ; --x) {
            for (y = (rows-3); y > 1; --y) {
                TS sum = TS(1) * dest[(y-2)*rowStep+x] + TS(3) * dest[(y-1)*rowStep+x] + TS(8) * dest[(y+0)*rowStep+x] + TS(3) * dest[(y+1)*rowStep+x] + TS(1) * dest[(y+2)*rowStep+x];
                T temp = T( (sum+half) / TS(16) );
                dest[(y*rowStep + x)] = temp;
            }
        }

    }
    check_add_2D(2,dest,rows,cols,rowStep,label);
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

// reverse just X
template <typename T, typename TS >
void convolution2D_sep_reverseX(const T *source, T *dest, int rows, int cols, int rowStep, const std::string &label) {
    int i;
    const bool isFloat = (T(2.9) > T(2.0));
    const TS half = isFloat ? TS(0) : TS(8);

    start_timer();

    for( i = 0; i < iterations; ++i ) {
        int x, y;

        // horizontal pass
        for (y = 0; y < rows; ++y) {
            for (x = (cols-3); x > 1 ; --x) {
                TS sum = TS(1) * source[(y*rowStep)+x-2] + TS(3) * source[(y*rowStep)+x-1] + TS(8) * source[(y*rowStep)+x+0] + TS(3) * source[(y*rowStep)+x+1] + TS(1) * source[(y*rowStep)+x+2];
                T temp = T( (sum+half) / TS(16) );
                dest[(y*rowStep + x)] = temp;
            }
        }

        // vertical pass
        for (x = (cols-3); x > 1 ; --x) {
            for (y = 2; y < (rows-2); ++y) {
                TS sum = TS(1) * dest[(y-2)*rowStep+x] + TS(3) * dest[(y-1)*rowStep+x] + TS(8) * dest[(y+0)*rowStep+x] + TS(3) * dest[(y+1)*rowStep+x] + TS(1) * dest[(y+2)*rowStep+x];
                T temp = T( (sum+half) / TS(16) );
                dest[(y*rowStep + x)] = temp;
            }
        }
    }
    check_add_2D(2,dest,rows,cols,rowStep,label);
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

// reverse just Y
template <typename T, typename TS >
void convolution2D_sep_reverseY(const T *source, T *dest, int rows, int cols, int rowStep, const std::string &label) {
    int i;
    const bool isFloat = (T(2.9) > T(2.0));
    const TS half = isFloat ? TS(0) : TS(8);

    start_timer();

    for( i = 0; i < iterations; ++i ) {
        int x, y;

        // horizontal pass
        for (y = rows-1; y >= 0; --y) {
            for (x = 2; x < (cols-2); ++x) {
                TS sum = TS(1) * source[(y*rowStep)+x-2] + TS(3) * source[(y*rowStep)+x-1] + TS(8) * source[(y*rowStep)+x+0] + TS(3) * source[(y*rowStep)+x+1] + TS(1) * source[(y*rowStep)+x+2];
                T temp = T( (sum+half) / TS(16) );
                dest[(y*rowStep + x)] = temp;
            }
        }

        // vertical pass
        for (x = 2; x < (cols-2); ++x) {
            for (y = (rows-3); y > 1; --y) {
                TS sum = TS(1) * dest[(y-2)*rowStep+x] + TS(3) * dest[(y-1)*rowStep+x] + TS(8) * dest[(y+0)*rowStep+x] + TS(3) * dest[(y+1)*rowStep+x] + TS(1) * dest[(y+2)*rowStep+x];
                T temp = T( (sum+half) / TS(16) );
                dest[(y*rowStep + x)] = temp;
            }
        }
    }
    check_add_2D(2,dest,rows,cols,rowStep,label);
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

// move loop invariant values out (loop invariant code motion)
// increment pointers as induction values
template <typename T, typename TS >
void convolution2D_sep_opt1(const T *source, T *dest, int rows, int cols, int rowStep, const std::string &label) {
    int i;
    const bool isFloat = (T(2.9) > T(2.0));
    const TS half = isFloat ? TS(0) : TS(8);

    start_timer();

    for( i = 0; i < iterations; ++i ) {
        int x, y;

        // horizontal pass
        const T *source2 = source - 2;
        T *dest2 = dest;
        for (y = 0; y < rows; ++y) {
            for (x = 2; x < (cols-2); ++x) {
                TS sum = TS(1) * source2[x] + TS(3) * source2[x+1] + TS(8) * source2[x+2] + TS(3) * source2[x+3] + TS(1) * source2[x+4];
                T temp = T( (sum+half) / TS(16) );
                dest2[x] = temp;
            }
            source2 += rowStep;
            dest2 += rowStep;
        }

        // vertical pass
        dest2 = dest + 2*rowStep;
        // using offset source ended up slower!?
        for (y = 2; y < (rows-2); ++y) {
            for (x = 2; x < (cols-2); ++x) {
                TS sum = TS(1) * dest2[-2*rowStep+x] + TS(3) * dest2[-1*rowStep+x] + TS(8) * dest2[0*rowStep+x] + TS(3) * dest2[1*rowStep+x] + TS(1) * dest2[2*rowStep+x];
                T temp = T( (sum+half) / TS(16) );
                dest2[x] = temp;
            }
            dest2 += rowStep;
        }

    }
    check_add_2D(2,dest,rows,cols,rowStep,label);
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

// move loop invariant values out (loop invariant code motion)
// increment pointers as induction values
// Unroll 4x in X
template <typename T, typename TS >
void convolution2D_sep_opt2(const T *source, T *dest, int rows, int cols, int rowStep, const std::string &label) {
    int i;
    const bool isFloat = (T(2.9) > T(2.0));
    const TS half = isFloat ? TS(0) : TS(8);

    start_timer();

    for( i = 0; i < iterations; ++i ) {
        int x, y;

        // horizontal pass
        const T *source2 = source;
        T *dest2 = dest;
        for (y = 0; y < rows; ++y) {
            for (x = 2; x < (cols-2-3); x+=4) {
                TS sum0 = TS(1) * source2[x-2] + TS(3) * source2[x-1] + TS(8) * source2[x+0] + TS(3) * source2[x+1] + TS(1) * source2[x+2];
                TS sum1 = TS(1) * source2[x-1] + TS(3) * source2[x+0] + TS(8) * source2[x+1] + TS(3) * source2[x+2] + TS(1) * source2[x+3];
                TS sum2 = TS(1) * source2[x+0] + TS(3) * source2[x+1] + TS(8) * source2[x+2] + TS(3) * source2[x+3] + TS(1) * source2[x+4];
                TS sum3 = TS(1) * source2[x+1] + TS(3) * source2[x+2] + TS(8) * source2[x+3] + TS(3) * source2[x+4] + TS(1) * source2[x+5];
                
                T temp0 = T( (sum0+half) / TS(16) );
                T temp1 = T( (sum1+half) / TS(16) );
                T temp2 = T( (sum2+half) / TS(16) );
                T temp3 = T( (sum3+half) / TS(16) );
                
                dest2[x+0] = temp0;
                dest2[x+1] = temp1;
                dest2[x+2] = temp2;
                dest2[x+3] = temp3;
            }
            for ( ; x < (cols-2); ++x) {
                TS sum = TS(1) * source2[x-2] + TS(3) * source2[x-1] + TS(8) * source2[x+0] + TS(3) * source2[x+1] + TS(1) * source2[x+2];
                T temp = T( (sum+half) / TS(16) );
                dest2[x] = temp;
            }
            source2 += rowStep;
            dest2 += rowStep;
        }

        // vertical pass
        dest2 = dest + 2*rowStep;
        // using offset source ended up slower!?
        for (y = 2; y < (rows-2); ++y) {
            x = 2;
            for (; x < (cols-2-3); x += 4) {
                TS sum0 = TS(1) * dest2[-2*rowStep+x+0] + TS(3) * dest2[-1*rowStep+x+0] + TS(8) * dest2[0*rowStep+x+0] + TS(3) * dest2[1*rowStep+x+0] + TS(1) * dest2[2*rowStep+x+0];
                TS sum1 = TS(1) * dest2[-2*rowStep+x+1] + TS(3) * dest2[-1*rowStep+x+1] + TS(8) * dest2[0*rowStep+x+1] + TS(3) * dest2[1*rowStep+x+1] + TS(1) * dest2[2*rowStep+x+1];
                TS sum2 = TS(1) * dest2[-2*rowStep+x+2] + TS(3) * dest2[-1*rowStep+x+2] + TS(8) * dest2[0*rowStep+x+2] + TS(3) * dest2[1*rowStep+x+2] + TS(1) * dest2[2*rowStep+x+2];
                TS sum3 = TS(1) * dest2[-2*rowStep+x+3] + TS(3) * dest2[-1*rowStep+x+3] + TS(8) * dest2[0*rowStep+x+3] + TS(3) * dest2[1*rowStep+x+3] + TS(1) * dest2[2*rowStep+x+3];
            
                T temp0 = T( (sum0+half) / TS(16) );
                T temp1 = T( (sum1+half) / TS(16) );
                T temp2 = T( (sum2+half) / TS(16) );
                T temp3 = T( (sum3+half) / TS(16) );
                
                dest2[x+0] = temp0;
                dest2[x+1] = temp1;
                dest2[x+2] = temp2;
                dest2[x+3] = temp3;
            }
            for (; x < (cols-2); ++x) {
                TS sum = TS(1) * dest2[-2*rowStep+x] + TS(3) * dest2[-1*rowStep+x] + TS(8) * dest2[0*rowStep+x] + TS(3) * dest2[1*rowStep+x] + TS(1) * dest2[2*rowStep+x];
                T temp = T( (sum+half) / TS(16) );
                dest2[x] = temp;
            }
            dest2 += rowStep;
        }

    }
    check_add_2D(2,dest,rows,cols,rowStep,label);
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

// move loop invariant values out (loop invariant code motion)
// increment pointers as induction values
// Unroll 8x in X
template <typename T, typename TS >
void convolution2D_sep_opt3(const T *source, T *dest, int rows, int cols, int rowStep, const std::string &label) {
    int i;
    const bool isFloat = (T(2.9) > T(2.0));
    const TS half = isFloat ? TS(0) : TS(8);

    start_timer();

    for( i = 0; i < iterations; ++i ) {
        int x, y;

        // horizontal pass
        const T *source2 = source;
        T *dest2 = dest;
        for (y = 0; y < rows; ++y) {
            const T *source3 = source2;
            T *dest3 = dest2;
            for (x = 2; x < (cols-2-7); x+=8) {
                TS sum0 = TS(1) * source3[-2] + TS(3) * source3[-1] + TS(8) * source3[0] + TS(3) * source3[1] + TS(1) * source3[2];
                TS sum1 = TS(1) * source3[-1] + TS(3) * source3[0] + TS(8) * source3[1] + TS(3) * source3[2] + TS(1) * source3[3];
                TS sum2 = TS(1) * source3[0] + TS(3) * source3[1] + TS(8) * source3[2] + TS(3) * source3[3] + TS(1) * source3[4];
                TS sum3 = TS(1) * source3[1] + TS(3) * source3[2] + TS(8) * source3[3] + TS(3) * source3[4] + TS(1) * source3[5];
                TS sum4 = TS(1) * source3[2] + TS(3) * source3[3] + TS(8) * source3[4] + TS(3) * source3[5] + TS(1) * source3[6];
                TS sum5 = TS(1) * source3[3] + TS(3) * source3[4] + TS(8) * source3[5] + TS(3) * source3[6] + TS(1) * source3[7];
                TS sum6 = TS(1) * source3[4] + TS(3) * source3[5] + TS(8) * source3[6] + TS(3) * source3[7] + TS(1) * source3[8];
                TS sum7 = TS(1) * source3[5] + TS(3) * source3[6] + TS(8) * source3[7] + TS(3) * source3[8] + TS(1) * source3[9];
                
                T temp0 = T( (sum0+half) / TS(16) );
                T temp1 = T( (sum1+half) / TS(16) );
                T temp2 = T( (sum2+half) / TS(16) );
                T temp3 = T( (sum3+half) / TS(16) );
                T temp4 = T( (sum4+half) / TS(16) );
                T temp5 = T( (sum5+half) / TS(16) );
                T temp6 = T( (sum6+half) / TS(16) );
                T temp7 = T( (sum7+half) / TS(16) );
                
                dest3[0] = temp0;
                dest3[1] = temp1;
                dest3[2] = temp2;
                dest3[3] = temp3;
                dest3[4] = temp4;
                dest3[5] = temp5;
                dest3[6] = temp6;
                dest3[7] = temp7;
                
                dest3 += 8;
                source3 += 8;
            }
            for ( ; x < (cols-2); ++x) {
                TS sum = TS(1) * source3[-2] + TS(3) * source3[-1] + TS(8) * source3[0] + TS(3) * source3[1] + TS(1) * source3[2];
                T temp = T( (sum+half) / TS(16) );
                dest3[0] = temp;
                dest3++;
                source3++;
            }
            source2 += rowStep;
            dest2 += rowStep;
        }


        // vertical pass
        dest2 = dest + 2*rowStep;
        // using offset source ended up slower!?
        for (y = 2; y < (rows-2); ++y) {
            x = 2;
            for (; x < (cols-2-7); x += 8) {
                TS sum0 = TS(1) * dest2[-2*rowStep+x+0] + TS(3) * dest2[-1*rowStep+x+0] + TS(8) * dest2[0*rowStep+x+0] + TS(3) * dest2[1*rowStep+x+0] + TS(1) * dest2[2*rowStep+x+0];
                TS sum1 = TS(1) * dest2[-2*rowStep+x+1] + TS(3) * dest2[-1*rowStep+x+1] + TS(8) * dest2[0*rowStep+x+1] + TS(3) * dest2[1*rowStep+x+1] + TS(1) * dest2[2*rowStep+x+1];
                TS sum2 = TS(1) * dest2[-2*rowStep+x+2] + TS(3) * dest2[-1*rowStep+x+2] + TS(8) * dest2[0*rowStep+x+2] + TS(3) * dest2[1*rowStep+x+2] + TS(1) * dest2[2*rowStep+x+2];
                TS sum3 = TS(1) * dest2[-2*rowStep+x+3] + TS(3) * dest2[-1*rowStep+x+3] + TS(8) * dest2[0*rowStep+x+3] + TS(3) * dest2[1*rowStep+x+3] + TS(1) * dest2[2*rowStep+x+3];
                TS sum4 = TS(1) * dest2[-2*rowStep+x+4] + TS(3) * dest2[-1*rowStep+x+4] + TS(8) * dest2[0*rowStep+x+4] + TS(3) * dest2[1*rowStep+x+4] + TS(1) * dest2[2*rowStep+x+4];
                TS sum5 = TS(1) * dest2[-2*rowStep+x+5] + TS(3) * dest2[-1*rowStep+x+5] + TS(8) * dest2[0*rowStep+x+5] + TS(3) * dest2[1*rowStep+x+5] + TS(1) * dest2[2*rowStep+x+5];
                TS sum6 = TS(1) * dest2[-2*rowStep+x+6] + TS(3) * dest2[-1*rowStep+x+6] + TS(8) * dest2[0*rowStep+x+6] + TS(3) * dest2[1*rowStep+x+6] + TS(1) * dest2[2*rowStep+x+6];
                TS sum7 = TS(1) * dest2[-2*rowStep+x+7] + TS(3) * dest2[-1*rowStep+x+7] + TS(8) * dest2[0*rowStep+x+7] + TS(3) * dest2[1*rowStep+x+7] + TS(1) * dest2[2*rowStep+x+7];
                
                T temp0 = T( (sum0+half) / TS(16) );
                T temp1 = T( (sum1+half) / TS(16) );
                T temp2 = T( (sum2+half) / TS(16) );
                T temp3 = T( (sum3+half) / TS(16) );
                T temp4 = T( (sum4+half) / TS(16) );
                T temp5 = T( (sum5+half) / TS(16) );
                T temp6 = T( (sum6+half) / TS(16) );
                T temp7 = T( (sum7+half) / TS(16) );
                
                dest2[0] = temp0;
                dest2[1] = temp1;
                dest2[2] = temp2;
                dest2[3] = temp3;
                dest2[4] = temp4;
                dest2[5] = temp5;
                dest2[6] = temp6;
                dest2[7] = temp7;
            }
            for (; x < (cols-2); ++x) {
                TS sum = TS(1) * dest2[-2*rowStep+x] + TS(3) * dest2[-1*rowStep+x] + TS(8) * dest2[0*rowStep+x] + TS(3) * dest2[1*rowStep+x] + TS(1) * dest2[2*rowStep+x];
                T temp = T( (sum+half) / TS(16) );
                dest2[x] = temp;
            }
            dest2 += rowStep;
        }

    }
    check_add_2D(2,dest,rows,cols,rowStep,label);
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

// move loop invariant values out (loop invariant code motion)
// increment pointers as induction values
// Unroll 8x in X
// make it look like a vector operation
template <typename T, typename TS >
void convolution2D_sep_opt4(const T *source, T *dest, int rows, int cols, int rowStep, const std::string &label) {
    int i;
    const bool isFloat = (T(2.9) > T(2.0));
    const TS half = isFloat ? TS(0) : TS(8);

    start_timer();

    for( i = 0; i < iterations; ++i ) {
        int x, y;

        // horizontal pass
        const T *source2 = source;
        T *dest2 = dest;
        for (y = 0; y < rows; ++y) {
            const T *source3 = source2;
            T *dest3 = dest2;
            for (x = 2; x < (cols-2-7); x+=8) {
                TS sum[8];
                T temp[8];
                sum[0] = TS(1) * source3[-2] + TS(3) * source3[-1] + TS(8) * source3[0] + TS(3) * source3[1] + TS(1) * source3[2];
                sum[1] = TS(1) * source3[-1] + TS(3) * source3[0] + TS(8) * source3[1] + TS(3) * source3[2] + TS(1) * source3[3];
                sum[2] = TS(1) * source3[0] + TS(3) * source3[1] + TS(8) * source3[2] + TS(3) * source3[3] + TS(1) * source3[4];
                sum[3] = TS(1) * source3[1] + TS(3) * source3[2] + TS(8) * source3[3] + TS(3) * source3[4] + TS(1) * source3[5];
                sum[4] = TS(1) * source3[2] + TS(3) * source3[3] + TS(8) * source3[4] + TS(3) * source3[5] + TS(1) * source3[6];
                sum[5] = TS(1) * source3[3] + TS(3) * source3[4] + TS(8) * source3[5] + TS(3) * source3[6] + TS(1) * source3[7];
                sum[6] = TS(1) * source3[4] + TS(3) * source3[5] + TS(8) * source3[6] + TS(3) * source3[7] + TS(1) * source3[8];
                sum[7] = TS(1) * source3[5] + TS(3) * source3[6] + TS(8) * source3[7] + TS(3) * source3[8] + TS(1) * source3[9];
                
                temp[0] = T( (sum[0]+half) / TS(16) );
                temp[1] = T( (sum[1]+half) / TS(16) );
                temp[2] = T( (sum[2]+half) / TS(16) );
                temp[3] = T( (sum[3]+half) / TS(16) );
                temp[4] = T( (sum[4]+half) / TS(16) );
                temp[5] = T( (sum[5]+half) / TS(16) );
                temp[6] = T( (sum[6]+half) / TS(16) );
                temp[7] = T( (sum[7]+half) / TS(16) );
                
                dest3[0] = temp[0];
                dest3[1] = temp[1];
                dest3[2] = temp[2];
                dest3[3] = temp[3];
                dest3[4] = temp[4];
                dest3[5] = temp[5];
                dest3[6] = temp[6];
                dest3[7] = temp[7];
                
                dest3 += 8;
                source3 += 8;
            }
            for ( ; x < (cols-2); ++x) {
                TS sum = TS(1) * source3[-2] + TS(3) * source3[-1] + TS(8) * source3[0] + TS(3) * source3[1] + TS(1) * source3[2];
                T temp = T( (sum+half) / TS(16) );
                dest3[0] = temp;
                dest3++;
                source3++;
            }
            source2 += rowStep;
            dest2 += rowStep;
        }


        // vertical pass
        dest2 = dest + 2*rowStep;
        source2 = dest2;
        // using offset source ended up slower!?
        for (y = 2; y < (rows-2); ++y) {
            x = 2;
            for (; x < (cols-2-7); x += 8) {
                TS sum[8];
                T temp[8];
                sum[0] = TS(1) * source2[-2*rowStep+x+0] + TS(3) * source2[-1*rowStep+x+0] + TS(8) * source2[0*rowStep+x+0] + TS(3) * source2[1*rowStep+x+0] + TS(1) * source2[2*rowStep+x+0];
                sum[1] = TS(1) * source2[-2*rowStep+x+1] + TS(3) * source2[-1*rowStep+x+1] + TS(8) * source2[0*rowStep+x+1] + TS(3) * source2[1*rowStep+x+1] + TS(1) * source2[2*rowStep+x+1];
                sum[2] = TS(1) * source2[-2*rowStep+x+2] + TS(3) * source2[-1*rowStep+x+2] + TS(8) * source2[0*rowStep+x+2] + TS(3) * source2[1*rowStep+x+2] + TS(1) * source2[2*rowStep+x+2];
                sum[3] = TS(1) * source2[-2*rowStep+x+3] + TS(3) * source2[-1*rowStep+x+3] + TS(8) * source2[0*rowStep+x+3] + TS(3) * source2[1*rowStep+x+3] + TS(1) * source2[2*rowStep+x+3];
                sum[4] = TS(1) * source2[-2*rowStep+x+4] + TS(3) * source2[-1*rowStep+x+4] + TS(8) * source2[0*rowStep+x+4] + TS(3) * source2[1*rowStep+x+4] + TS(1) * source2[2*rowStep+x+4];
                sum[5] = TS(1) * source2[-2*rowStep+x+5] + TS(3) * source2[-1*rowStep+x+5] + TS(8) * source2[0*rowStep+x+5] + TS(3) * source2[1*rowStep+x+5] + TS(1) * source2[2*rowStep+x+5];
                sum[6] = TS(1) * source2[-2*rowStep+x+6] + TS(3) * source2[-1*rowStep+x+6] + TS(8) * source2[0*rowStep+x+6] + TS(3) * source2[1*rowStep+x+6] + TS(1) * source2[2*rowStep+x+6];
                sum[7] = TS(1) * source2[-2*rowStep+x+7] + TS(3) * source2[-1*rowStep+x+7] + TS(8) * source2[0*rowStep+x+7] + TS(3) * source2[1*rowStep+x+7] + TS(1) * source2[2*rowStep+x+7];
                
                temp[0] = T( (sum[0]+half) / TS(16) );
                temp[1] = T( (sum[1]+half) / TS(16) );
                temp[2] = T( (sum[2]+half) / TS(16) );
                temp[3] = T( (sum[3]+half) / TS(16) );
                temp[4] = T( (sum[4]+half) / TS(16) );
                temp[5] = T( (sum[5]+half) / TS(16) );
                temp[6] = T( (sum[6]+half) / TS(16) );
                temp[7] = T( (sum[7]+half) / TS(16) );
                
                dest2[0] = temp[0];
                dest2[1] = temp[1];
                dest2[2] = temp[2];
                dest2[3] = temp[3];
                dest2[4] = temp[4];
                dest2[5] = temp[5];
                dest2[6] = temp[6];
                dest2[7] = temp[7];
            }
            for (; x < (cols-2); ++x) {
                TS sum = TS(1) * dest2[-2*rowStep+x] + TS(3) * dest2[-1*rowStep+x] + TS(8) * dest2[0*rowStep+x] + TS(3) * dest2[1*rowStep+x] + TS(1) * dest2[2*rowStep+x];
                T temp = T( (sum+half) / TS(16) );
                dest2[x] = temp;
            }
            dest2 += rowStep;
        }

    }
    check_add_2D(2,dest,rows,cols,rowStep,label);
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/
/******************************************************************************/

template< typename T, typename TS >
void TestOneType()
{
    const int base_iterations = iterations;

    std::string myTypeName( getTypeName<T>() );
    std::string myTypeNameA( getTypeName<TS>() );
    
    gLabels.clear();

#if 0
    // if these are stack arrays, MSVC silently exits the app after int16
    T data_flat[ HEIGHT*WIDTH ];
    T data_flatDst[ HEIGHT*WIDTH ];
#else
    std::unique_ptr<T> data_flat_unique( new T[ HEIGHT*WIDTH ] );
    std::unique_ptr<T> data_flatDst_unique( new T[ HEIGHT*WIDTH ] );
    T *data_flat = data_flat_unique.get();
    T *data_flatDst = data_flatDst_unique.get();
#endif

    ::fill( data_flat, data_flat+SIZE, T(init_value) );


    ::fill( data_flatDst, data_flatDst+SIZE, T(init_value+2) );
    convolution1D<T,TS>        ( data_flat, data_flatDst, SIZE, myTypeName + " convolution 1D");
    convolution1D_reverse<T,TS>( data_flat, data_flatDst, SIZE, myTypeName + " convolution 1D reverse");
    convolution1D_opt1<T,TS>   ( data_flat, data_flatDst, SIZE, myTypeName + " convolution 1D opt1");
    convolution1D_opt2<T,TS>   ( data_flat, data_flatDst, SIZE, myTypeName + " convolution 1D opt2");
    convolution1D_opt3<T,TS>   ( data_flat, data_flatDst, SIZE, myTypeName + " convolution 1D opt3");
    convolution1D_opt4<T,TS>   ( data_flat, data_flatDst, SIZE, myTypeName + " convolution 1D opt4");
    convolution1D_opt5<T,TS>   ( data_flat, data_flatDst, SIZE, myTypeName + " convolution 1D opt5");
    convolution1D_opt6<T,TS>   ( data_flat, data_flatDst, SIZE, myTypeName + " convolution 1D opt6");
    
    std::string temp1( myTypeName + " convolution 1D" );
    summarize( temp1.c_str(), SIZE, iterations, kDontShowGMeans, kDontShowPenalty );
    
    
    
    iterations = base_iterations / 2;
    
    ::fill( data_flatDst, data_flatDst+SIZE, T(init_value+3) );
    convolution2D<T,TS>         ( data_flat, data_flatDst, HEIGHT, WIDTH, WIDTH, myTypeName + " convolution 2D");
    convolution2D_reverse<T,TS> ( data_flat, data_flatDst, HEIGHT, WIDTH, WIDTH, myTypeName + " convolution 2D reverse");
    convolution2D_reverseX<T,TS>( data_flat, data_flatDst, HEIGHT, WIDTH, WIDTH, myTypeName + " convolution 2D reverseX");
    convolution2D_reverseY<T,TS>( data_flat, data_flatDst, HEIGHT, WIDTH, WIDTH, myTypeName + " convolution 2D reverseY");
    convolution2D_loopswap<T,TS>( data_flat, data_flatDst, HEIGHT, WIDTH, WIDTH, myTypeName + " convolution 2D loop swap");
    convolution2D_opt1<T,TS>    ( data_flat, data_flatDst, HEIGHT, WIDTH, WIDTH, myTypeName + " convolution 2D opt1");
    convolution2D_opt2<T,TS>    ( data_flat, data_flatDst, HEIGHT, WIDTH, WIDTH, myTypeName + " convolution 2D opt2");
    convolution2D_opt3<T,TS>    ( data_flat, data_flatDst, HEIGHT, WIDTH, WIDTH, myTypeName + " convolution 2D opt3");
    convolution2D_opt4<T,TS>    ( data_flat, data_flatDst, HEIGHT, WIDTH, WIDTH, myTypeName + " convolution 2D opt4");
    convolution2D_opt5<T,TS>    ( data_flat, data_flatDst, HEIGHT, WIDTH, WIDTH, myTypeName + " convolution 2D opt5");
    convolution2D_opt6<T,TS>    ( data_flat, data_flatDst, HEIGHT, WIDTH, WIDTH, myTypeName + " convolution 2D opt6");
    convolution2D_opt7<T,TS>    ( data_flat, data_flatDst, HEIGHT, WIDTH, WIDTH, myTypeName + " convolution 2D opt7");
    convolution2D_opt8<T,TS>    ( data_flat, data_flatDst, HEIGHT, WIDTH, WIDTH, myTypeName + " convolution 2D opt8");
    convolution2D_opt9<T,TS>    ( data_flat, data_flatDst, HEIGHT, WIDTH, WIDTH, myTypeName + " convolution 2D opt9");
    
    std::string temp2( myTypeName + " convolution 2D" );
    summarize( temp2.c_str(), SIZE, iterations, kDontShowGMeans, kDontShowPenalty );


    
    iterations = base_iterations / 2;

    ::fill( data_flatDst, data_flatDst+SIZE, T(init_value+4) );
    convolution2D_sep<T,TS>              ( data_flat, data_flatDst, HEIGHT, WIDTH, WIDTH, myTypeName + " convolution 2D separable");
    convolution2D_sep_reverse<T,TS>      ( data_flat, data_flatDst, HEIGHT, WIDTH, WIDTH, myTypeName + " convolution 2D separable reverse");
    convolution2D_sep_reverseX<T,TS>     ( data_flat, data_flatDst, HEIGHT, WIDTH, WIDTH, myTypeName + " convolution 2D separable reverse X");
    convolution2D_sep_reverseY<T,TS>     ( data_flat, data_flatDst, HEIGHT, WIDTH, WIDTH, myTypeName + " convolution 2D separable reverse Y");
    convolution2D_sep_swap<T,TS>         ( data_flat, data_flatDst, HEIGHT, WIDTH, WIDTH, myTypeName + " convolution 2D separable swapped");
    convolution2D_sep_swaphoriz<T,TS>    ( data_flat, data_flatDst, HEIGHT, WIDTH, WIDTH, myTypeName + " convolution 2D separable swapped horiz");
    convolution2D_sep_swapvert<T,TS>     ( data_flat, data_flatDst, HEIGHT, WIDTH, WIDTH, myTypeName + " convolution 2D separable swapped vert");
    convolution2D_sep_opt1<T,TS>         ( data_flat, data_flatDst, HEIGHT, WIDTH, WIDTH, myTypeName + " convolution 2D separable opt1");
    convolution2D_sep_opt2<T,TS>         ( data_flat, data_flatDst, HEIGHT, WIDTH, WIDTH, myTypeName + " convolution 2D separable opt2");
    convolution2D_sep_opt3<T,TS>         ( data_flat, data_flatDst, HEIGHT, WIDTH, WIDTH, myTypeName + " convolution 2D separable opt3");
    convolution2D_sep_opt4<T,TS>         ( data_flat, data_flatDst, HEIGHT, WIDTH, WIDTH, myTypeName + " convolution 2D separable opt4");
    
    std::string temp3( myTypeName + " convolution 2D separable" );
    summarize( temp3.c_str(), SIZE, iterations, kDontShowGMeans, kDontShowPenalty );
    
    iterations = base_iterations;

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


    TestOneType<uint8_t, uint16_t>();
    TestOneType<int8_t, int16_t>();     // signed 8 bit isn't very useful, but does show compiler problems
    
    TestOneType<uint16_t, uint32_t>();
    TestOneType<int16_t, int32_t>();

    iterations /= 2;

    TestOneType<uint32_t, uint64_t>();
    TestOneType<int32_t, int64_t>();
    
    TestOneType<uint64_t, uint64_t>();
    TestOneType<int64_t, int64_t>();

    TestOneType<float,float>();
    TestOneType<double,double>();

#if WORKS_BUT_SLOW
    TestOneType<long double,long double>();
#endif
    
    return 0;
}

// the end
/******************************************************************************/
/******************************************************************************/
