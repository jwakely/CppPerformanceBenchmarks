/*
    Copyright 2019 Chris Cox
    Distributed under the MIT License (see accompanying file LICENSE_1_0_0.txt
    or a copy at http://stlab.adobe.com/licenses.html )


Goal:  Test compiler optimizations related to box filter convolution operations.
        (common in imaging, audio, simulation, and scientific computations)
 

Assumptions:

    1) Most likely, there will be no single best implementation for all types.
        "Best" performance depends a lot on cache organization, instruction latencies, and register availability.





TODO - find best buffer sizes
        need to test in and out of cache

TODO -
    3D box filters
    iterated box filters

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
#include <numeric>
#include <iostream>
#include <algorithm>
#include "benchmark_results.h"
#include "benchmark_timer.h"
#include "benchmark_typenames.h"

/******************************************************************************/

// this constant may need to be adjusted to give reasonable minimum times
// For best results, times should be about 1.0 seconds for the minimum test run
int iterations = 400;


// ~ 2 million items (src plus dest), intended to be larger than L2 cache on common CPUs
const int WIDTH = 1200;
const int HEIGHT = 900;

const int SIZE = HEIGHT*WIDTH;


// initial value for filling our arrays, may be changed from the command line
double init_value = 3.0;

/******************************************************************************/

std::deque<std::string> gLabels;

/******************************************************************************/

#include "benchmark_shared_tests.h"

/******************************************************************************/

template <typename T>
inline void check_add(const T* out, const int rows, const int cols, const int rowStep, const std::string &label) {
    T sum = 0;
    
    for (int y = 0; y < rows; ++y)
        for(int x = 0; x < cols; ++x)
            sum += out[y*rowStep+x];
    
    T temp = (T)(cols*rows) * (T)init_value;
    if (!tolerance_equal<T>(sum,temp))
        std::cout << "test " << label << " failed, got " << sum << " expected " << temp << "\n";
}

/******************************************************************************/
/******************************************************************************/

// Simple horizontal box filter with edge replication

// O(N)
template<typename T, typename TS>
void box_horiz(const T *src, T *dest, int rows, int cols, int rowStep, int edge ) {
    const bool isFloat = (T(2.9) > T(2.0));
    const TS half = isFloat ? TS(0) : TS(edge/2);
    int halfEdge = edge / 2;
    int remainEdge = edge - halfEdge;
    
    // assert( cols >= edge );
    
    for (int y = 0; y < rows; ++y) {
        const T *srcRow = src + y*rowStep;
        T *destRow = dest + y*rowStep;
        
        for (int x = 0; x < cols; ++x) {
            TS sum = half;
            
            for (int k = -halfEdge; k < remainEdge; ++k) {
                if ((x+k) < 0)
                    sum += srcRow[0];
                else if ((x+k) >= cols)
                    sum += srcRow[cols-1];
                else
                    sum += srcRow[(x+k)];
            }
            
            sum = sum / edge;
            
            destRow[x] = T(sum);
        }
    }
}

/******************************************************************************/

// Simple horizontal box filter with edge replication
// move the edge conditions into separate loops

// O(N)
template<typename T, typename TS>
void box_horiz_opt1(const T *src, T *dest, int rows, int cols, int rowStep, int edge ) {
    const bool isFloat = (T(2.9) > T(2.0));
    const TS half = isFloat ? TS(0) : TS(edge/2);
    int halfEdge = edge / 2;
    int remainEdge = edge - halfEdge;
    
    // assert( cols >= edge );
    
    for (int y = 0; y < rows; ++y) {
        const T *srcRow = src + y*rowStep;
        T *destRow = dest + y*rowStep;
        
        for (int x = 0; x < cols; ++x) {
            TS sum = half;
            int start = x-halfEdge;
            int end = x+remainEdge;
            int mid = std::min(cols,end);
            int k;

            for ( ; start < 0; ++start )
                sum += srcRow[0];
            
            for (k = start; k < mid; ++k)
                sum += srcRow[k];
            
            for ( ; k < end; ++k)
                sum += srcRow[cols-1];
            
            sum = sum / edge;
            
            destRow[x] = T(sum);
        }
    }
}

/******************************************************************************/

// Simple horizontal box filter with edge replication
// break out edge conditions into larger loops, only test edge conditions near edges

// O(N)
template<typename T, typename TS>
void box_horiz_opt2(const T *src, T *dest, int rows, int cols, int rowStep, int edge ) {
    const bool isFloat = (T(2.9) > T(2.0));
    const TS half = isFloat ? TS(0) : TS(edge/2);
    const int halfEdge = edge / 2;
    const int remainEdge = edge - halfEdge;
    const int start_edge = std::min(halfEdge,cols);
    const int end_edge = cols - std::min(remainEdge,cols);
    
    // assert( cols >= edge );
    
    for (int y = 0; y < rows; ++y) {
        const T *srcRow = src + y*rowStep;
        T *destRow = dest + y*rowStep;
        int x;
        
        for (x = 0; x < start_edge; ++x) {
            TS sum = half;
            int start = x-halfEdge;
            int end = x+remainEdge;
            int mid = std::min(cols,end);
            int k;

            for ( ; start < 0; ++start )
                sum += srcRow[0];
            
            for (k = start; k < mid; ++k)
                sum += srcRow[k];
            
            for ( ; k < end; ++k)
                sum += srcRow[cols-1];
            
            sum = sum / edge;
            
            destRow[x] = T(sum);
        }
        
        for (; x < end_edge; ++x) {
            TS sum = half;
            
            for (int k = -halfEdge; k < remainEdge; ++k) {
                sum += srcRow[(x+k)];
            }
            
            sum = sum / edge;
            
            destRow[x] = T(sum);
        }
        
        
        for (; x < cols; ++x) {
            TS sum = half;
            int start = x-halfEdge;
            int end = x+remainEdge;
            int mid = std::min(cols,end);
            int k;

            for ( ; start < 0; ++start )
                sum += srcRow[0];
            
            for (k = start; k < mid; ++k)
                sum += srcRow[k];
            
            for ( ; k < end; ++k)
                sum += srcRow[cols-1];
            
            sum = sum / edge;
            
            destRow[x] = T(sum);
        }
    }
}

/******************************************************************************/

// Simple horizontal box filter with edge replication
// break out edge conditions into larger loops
// stop doing redundant adds

// O(1)
template<typename T, typename TS>
void box_horiz_opt3(const T *src, T *dest, int rows, int cols, int rowStep, int edge ) {
    const bool isFloat = (T(2.9) > T(2.0));
    const TS half = isFloat ? TS(0) : TS(edge/2);
    const int halfEdge = edge / 2;
    const int remainEdge = edge - halfEdge;
    const int start_edge = std::min(halfEdge,cols-1);
    const int end_edge = (cols-1) - std::min(remainEdge,cols-1);
    
    // assert( cols >= edge );
    
    for (int y = 0; y < rows; ++y) {
        const T *srcRow = src + y*rowStep;
        T *destRow = dest + y*rowStep;
        int x, k;
        TS sum = half;
        
        
        // sum for first pixel
        for (k = -halfEdge; k < 0; ++k) {
            sum += srcRow[0];
        }
        for (; k < remainEdge; ++k) {
            sum += srcRow[k];
        }
        
        for (x = 0; x < start_edge; ++x) {
            TS result = sum / edge;
            destRow[x] = T(result);
            
            // subtract old value, add new value
            // assert( (x - halfEdge) <= 0 );
            sum += srcRow[x+remainEdge];
            sum -= srcRow[0];
        }
        
        for (; x < end_edge; ++x) {
            TS result = sum / edge;
            destRow[x] = T(result);
            
            // subtract old value, add new value
            sum += srcRow[x+remainEdge];
            sum -= srcRow[x-halfEdge];
        }
        
        for (; x < (cols-1); ++x) {
            TS result = sum / edge;
            destRow[x] = T(result);
            
            // subtract old value, add new value
            // assert( (x+remainEdge) >= (cols-1) );
            sum += srcRow[(cols-1)];
            sum -= srcRow[x-halfEdge];
        }
        
        
        // output last pixel
            sum = sum / edge;
            destRow[(cols-1)] = T(sum);
        
    }
}

/******************************************************************************/

// Simple horizontal box filter with edge replication
// break out edge conditions into larger loops
// stop doing redundant adds
// unroll some loops

// O(1)
template<typename T, typename TS>
void box_horiz_opt4(const T *src, T *dest, int rows, int cols, int rowStep, int edge ) {
    const bool isFloat = (T(2.9) > T(2.0));
    const TS half = isFloat ? TS(0) : TS(edge/2);
    const int halfEdge = edge / 2;
    const int remainEdge = edge - halfEdge;
    const int start_edge = std::min(halfEdge,cols-1);
    const int end_edge = (cols-1) - std::min(remainEdge,cols-1);
    
    // assert( cols >= edge );
    
    for (int y = 0; y < rows; ++y) {
        const T *srcRow = src + y*rowStep;
        T *destRow = dest + y*rowStep;
        int x, k;
        TS sum = half;
        
        
        // sum for first pixel
        sum += halfEdge * srcRow[0];
        k = 0;
        for (; k < remainEdge; ++k) {
            sum += srcRow[k];
        }
        
        for (x = 0; x < start_edge; ++x) {
            TS result = sum / edge;
            destRow[x] = T(result);
            
            // subtract old value, add new value
            // assert( (x - halfEdge) <= 0 );
            sum += srcRow[x+remainEdge];
            sum -= srcRow[0];
        }

        for (; x < (end_edge-3); x += 4) {
            TS result0 = sum / edge;
            sum += srcRow[x+0+remainEdge];
            sum -= srcRow[x+0-halfEdge];
            TS result1 = sum / edge;
            sum += srcRow[x+1+remainEdge];
            sum -= srcRow[x+1-halfEdge];
            TS result2 = sum / edge;
            sum += srcRow[x+2+remainEdge];
            sum -= srcRow[x+2-halfEdge];
            TS result3 = sum / edge;
            sum += srcRow[x+3+remainEdge];
            sum -= srcRow[x+3-halfEdge];
            
            destRow[x+0] = T(result0);
            destRow[x+1] = T(result1);
            destRow[x+2] = T(result2);
            destRow[x+3] = T(result3);
        }
        
        for (; x < end_edge; ++x) {
            TS result = sum / edge;
            destRow[x] = T(result);
            
            // subtract old value, add new value
            sum += srcRow[x+remainEdge];
            sum -= srcRow[x-halfEdge];
        }
        
        for (; x < (cols-1); ++x) {
            TS result = sum / edge;
            destRow[x] = T(result);
            
            // subtract old value, add new value
            // assert( (x+remainEdge) >= (cols-1) );
            sum += srcRow[(cols-1)];
            sum -= srcRow[x-halfEdge];
        }
        
        
        // output last pixel
            sum = sum / edge;
            destRow[(cols-1)] = T(sum);
        
    }
}

/******************************************************************************/
/******************************************************************************/

// Simple horizontal box filter without edge conditions (prepadded buffer)

// O(N)
template<typename T, typename TS>
void box_horiz_pad(const T *src, T *dest, int rows, int cols, int rowStep, int edge ) {
    const bool isFloat = (T(2.9) > T(2.0));
    const TS half = isFloat ? TS(0) : TS(edge/2);
    
    // assert( cols >= edge );
    
    for (int y = 0; y < rows; ++y) {
        const T *srcRow = src + y*rowStep;
        T *destRow = dest + y*rowStep;
        
        for (int x = 0; x < cols; ++x) {
            TS sum = half;
            
            for (int k = 0; k < edge; ++k)
                sum += srcRow[(x+k)];
            
            sum = sum / edge;
            
            destRow[x] = T(sum);
        }
    }
}

/******************************************************************************/

// Simple horizontal box filter without edge conditions (prepadded buffer)
// unroll inner loop

// O(N)
template<typename T, typename TS>
void box_horiz_pad_opt1(const T *src, T *dest, int rows, int cols, int rowStep, int edge ) {
    const bool isFloat = (T(2.9) > T(2.0));
    const TS half = isFloat ? TS(0) : TS(edge/2);
    
    // assert( cols >= edge );
    
    for (int y = 0; y < rows; ++y) {
        const T *srcRow = src + y*rowStep;
        T *destRow = dest + y*rowStep;
        
        for (int x = 0; x < cols; ++x) {
            TS sum = half;
            int k = 0;
            
            for (; k < (edge-4); k += 4) {
                sum += srcRow[(x+k+0)];
                sum += srcRow[(x+k+1)];
                sum += srcRow[(x+k+2)];
                sum += srcRow[(x+k+3)];
            }
            
            for (; k < edge; ++k) {
                sum += srcRow[(x+k+0)];
            }
            
            sum = sum / edge;
            
            destRow[x] = T(sum);
        }
    }
}

/******************************************************************************/

// Simple horizontal box filter without edge conditions (prepadded buffer)
// unroll inner loop
// use temp that looks like a vector

// O(N)
template<typename T, typename TS>
void box_horiz_pad_opt2(const T *src, T *dest, int rows, int cols, int rowStep, int edge ) {
    const bool isFloat = (T(2.9) > T(2.0));
    const TS half = isFloat ? TS(0) : TS(edge/2);
    
    // assert( cols >= edge );
    
    for (int y = 0; y < rows; ++y) {
        const T *srcRow = src + y*rowStep;
        T *destRow = dest + y*rowStep;
        
        for (int x = 0; x < cols; ++x) {
            TS sum = half;
            int k = 0;
            TS temp[8] = { 0,0,0,0, 0,0,0,0 };
            
            for (; k < (edge-7); k += 8) {
                temp[0] += srcRow[(x+k+0)];
                temp[1] += srcRow[(x+k+1)];
                temp[2] += srcRow[(x+k+2)];
                temp[3] += srcRow[(x+k+3)];
                temp[4] += srcRow[(x+k+4)];
                temp[5] += srcRow[(x+k+5)];
                temp[6] += srcRow[(x+k+6)];
                temp[7] += srcRow[(x+k+7)];
            }
            
            for (; k < edge; ++k) {
                sum += srcRow[(x+k+0)];
            }
            
            temp[0] += temp[1] + temp[2] + temp[3];
            temp[4] += temp[5] + temp[6] + temp[7];
            sum += temp[0] + temp[4];
            
            sum = sum / edge;
            
            destRow[x] = T(sum);
        }
    }
}

/******************************************************************************/

// Simple horizontal box filter without edge conditions (prepadded buffer)
// stop doing redundant adds

// O(1)
template<typename T, typename TS>
void box_horiz_pad_opt3(const T *src, T *dest, int rows, int cols, int rowStep, int edge ) {
    const bool isFloat = (T(2.9) > T(2.0));
    const TS half = isFloat ? TS(0) : TS(edge/2);
    
    // assert( cols >= edge );
    
    for (int y = 0; y < rows; ++y) {
        const T *srcRow = src + y*rowStep;
        T *destRow = dest + y*rowStep;
        TS sum = half;
        
        // sum for first pixel
        for (int k = 0; k < edge; ++k) {
            sum += srcRow[k];
        }
    
        for (int x = 0; x < (cols-1); ++x) {
            TS result = sum / edge;
            destRow[x] = T(result);
            
            sum += srcRow[x+edge];
            sum -= srcRow[x];
        }
        
        // output last value
        TS result = sum / edge;
        destRow[(cols-1)] = T(result);
        
    }
}

/******************************************************************************/

// Simple horizontal box filter without edge conditions (prepadded buffer)
// stop doing redundant adds
// unroll inner loops

// O(1)
template<typename T, typename TS>
void box_horiz_pad_opt4(const T *src, T *dest, int rows, int cols, int rowStep, int edge ) {
    const bool isFloat = (T(2.9) > T(2.0));
    const TS half = isFloat ? TS(0) : TS(edge/2);
    const T *srcRow = src;
    T *destRow = dest;
    
    // assert( cols >= edge );
    
    for (int y = 0; y < rows; ++y) {
        TS sum = half;
        //TS temp[4] = { 0,0,0,0 };
        
        // sum for first pixel
        int k = 0;
        for (; k < (edge-3); k += 4) {
            sum += srcRow[k+0];
            sum += srcRow[k+1];
            sum += srcRow[k+2];
            sum += srcRow[k+3];
        }
        for (; k < edge; ++k) {
            sum += srcRow[k];
        }
    
        // middle pixel values
        int x = 0;
        for (; x < (cols-4); x += 4) {
            TS result0 = sum / edge;
            sum += srcRow[x+0+edge];
            sum -= srcRow[x+0];
            TS result1 = sum / edge;
            sum += srcRow[x+1+edge];
            sum -= srcRow[x+1];
            TS result2 = sum / edge;
            sum += srcRow[x+2+edge];
            sum -= srcRow[x+2];
            TS result3 = sum / edge;
            sum += srcRow[x+3+edge];
            sum -= srcRow[x+3];
        
            destRow[x+0] = T(result0);
            destRow[x+1] = T(result1);
            destRow[x+2] = T(result2);
            destRow[x+3] = T(result3);
        }
        
        for (; x < (cols-1); ++x) {
            TS result = sum / edge;
            destRow[x] = T(result);
            
            sum += srcRow[x+edge];
            sum -= srcRow[x];
        }
        
        // output last value
        TS result = sum / edge;
        destRow[(cols-1)] = T(result);
        
        srcRow += rowStep;
        destRow += rowStep;
    }

}

/******************************************************************************/
/******************************************************************************/

// Simple vertical box filter with edge replication
// a great way to thrash the cache and TLBs

// O(N)
template<typename T, typename TS>
void box_vert(const T *src, T *dest, int rows, int cols, int rowStep, int edge ) {
    const bool isFloat = (T(2.9) > T(2.0));
    const TS half = isFloat ? TS(0) : TS(edge/2);
    int halfEdge = edge / 2;
    int remainEdge = edge - halfEdge;
    
    // assert( rows >= edge );
    
    for (int x = 0; x < cols; ++x) {
        for (int y = 0; y < rows; ++y) {
            TS sum = half;
            
            for (int k = -halfEdge; k < remainEdge; ++k) {
                if ((y+k) < 0)
                    sum += src[0*rowStep+x];
                else if ((y+k) >= rows)
                    sum += src[(rows-1)*rowStep+x];
                else
                    sum += src[(y+k)*rowStep+x];
            }
            
            sum = sum / edge;
            
            dest[y*rowStep+x] = T(sum);
        }
    }
}

/******************************************************************************/

// Simple vertical box filter with edge replication
// a great way to thrash the cache and TLBs
// move the edge conditions into separate loops

// O(N)
template<typename T, typename TS>
void box_vert_opt1(const T *src, T *dest, int rows, int cols, int rowStep, int edge ) {
    const bool isFloat = (T(2.9) > T(2.0));
    const TS half = isFloat ? TS(0) : TS(edge/2);
    int halfEdge = edge / 2;
    int remainEdge = edge - halfEdge;
    
    // assert( rows >= edge );
    
    for (int x = 0; x < cols; ++x) {
        for (int y = 0; y < rows; ++y) {
            TS sum = half;
            int start = y-halfEdge;
            int end = y+remainEdge;
            int mid = std::min(rows,end);
            int k;

            for ( ; start < 0; ++start )
                sum += src[0*rowStep+x];
            
            for (k = start; k < mid; ++k)
                sum += src[k*rowStep+x];
            
            for ( ; k < end; ++k)
                sum += src[(rows-1)*rowStep+x];
            
            sum = sum / edge;
            
            dest[y*rowStep+x] = T(sum);
        }
    }
}

/******************************************************************************/

// Simple vertical box filter with edge replication
// a great way to thrash the cache and TLBs
// break out edge conditions into larger loops, only test edge conditions near edges

// O(N)
template<typename T, typename TS>
void box_vert_opt2(const T *src, T *dest, int rows, int cols, int rowStep, int edge ) {
    const bool isFloat = (T(2.9) > T(2.0));
    const TS half = isFloat ? TS(0) : TS(edge/2);
    const int halfEdge = edge / 2;
    const int remainEdge = edge - halfEdge;
    const int start_edge = std::min(halfEdge,rows);
    const int end_edge = rows - std::min(remainEdge,rows);
    
    // assert( rows >= edge );
    
    for (int x = 0; x < cols; ++x) {
        int y;
        
        for (y = 0; y < start_edge; ++y) {
            TS sum = half;
            int start = y-halfEdge;
            int end = y+remainEdge;
            int mid = std::min(rows,end);
            int k;

            for ( ; start < 0; ++start )
                sum += src[0*rowStep+x];
            
            for (k = start; k < mid; ++k)
                sum += src[k*rowStep+x];
            
            for ( ; k < end; ++k)
                sum += src[(rows-1)*rowStep+x];
            
            sum = sum / edge;
            
            dest[y*rowStep+x] = T(sum);

        }
        
        for (; y < end_edge; ++y) {
            TS sum = half;
            int start = y-halfEdge;
            int end = y+remainEdge;
            
            for (int k = start; k < end; ++k)
                sum += src[k*rowStep+x];
            
            sum = sum / edge;
            
            dest[y*rowStep+x] = T(sum);
        }
        
        for (; y < rows; ++y) {
            TS sum = half;
            int start = y-halfEdge;
            int end = y+remainEdge;
            int mid = std::min(rows,end);
            int k;

            for ( ; start < 0; ++start )
                sum += src[0*rowStep+x];
            
            for (k = start; k < mid; ++k)
                sum += src[k*rowStep+x];
            
            for ( ; k < end; ++k)
                sum += src[(rows-1)*rowStep+x];
            
            sum = sum / edge;
            
            dest[y*rowStep+x] = T(sum);
        }
    
    }
}

/******************************************************************************/

// Simple vertical box filter with edge replication
// break out edge conditions into larger loops, only test edge conditions near edges
// stop doing redundant adds

// O(1)
template<typename T, typename TS>
void box_vert_opt3(const T *src, T *dest, int rows, int cols, int rowStep, int edge ) {
    const bool isFloat = (T(2.9) > T(2.0));
    const TS half = isFloat ? TS(0) : TS(edge/2);
    const int halfEdge = edge / 2;
    const int remainEdge = edge - halfEdge;
    const int start_edge = std::min(halfEdge,(rows-1));
    const int end_edge = (rows-1) - std::min(remainEdge,(rows-1));
    
    // assert( rows >= edge );
    
    for (int x = 0; x < cols; ++x) {
        int y, k;
        TS sum = half;
        
        
        // sum for first pixel
        for (k = -halfEdge; k < 0; ++k) {
            sum += src[0*rowStep+x];
        }
        for (; k < remainEdge; ++k) {
            sum += src[k*rowStep+x];
        }
        
        for (y = 0; y < start_edge; ++y) {
            TS result = sum / edge;
            dest[y*rowStep+x] = T(result);
            
            // subtract old value, add new value
            // assert( (y - halfEdge) <= 0 );
            sum += src[(y+remainEdge)*rowStep+x];
            sum -= src[0*rowStep+x];
        }
        
        for (; y < end_edge; ++y) {
            TS result = sum / edge;
            dest[y*rowStep+x] = T(result);
            
            // subtract old value, add new value
            sum += src[(y+remainEdge)*rowStep+x];
            sum -= src[(y-halfEdge)*rowStep+x];
        }
        
        for (; y < (rows-1); ++y) {
            TS result = sum / edge;
            dest[y*rowStep+x] = T(result);
            
            // subtract old value, add new value
            sum += src[(rows-1)*rowStep+x];
            sum -= src[(y-halfEdge)*rowStep+x];
        }
        
        // write last pixel
        sum = sum / edge;
        dest[(rows-1)*rowStep+x] = T(sum);
    
    }
}

/******************************************************************************/

// Simple vertical box filter with edge replication
// break out edge conditions into larger loops, only test edge conditions near edges
// stop doing redundant adds
// unroll some obvious, but wrong loops

// O(1)
template<typename T, typename TS>
void box_vert_opt4(const T *src, T *dest, int rows, int cols, int rowStep, int edge ) {
    const bool isFloat = (T(2.9) > T(2.0));
    const TS half = isFloat ? TS(0) : TS(edge/2);
    const int halfEdge = edge / 2;
    const int remainEdge = edge - halfEdge;
    const int start_edge = std::min(halfEdge,(rows-1));
    const int end_edge = (rows-1) - std::min(remainEdge,(rows-1));
    
    // assert( rows >= edge );
    
    for (int x = 0; x < cols; ++x) {
        int y, k;
        TS sum = half;
        
        
        // sum for first pixel
        sum += halfEdge * src[0*rowStep+x];
        k = 0;
        for (; k < remainEdge; ++k) {
            sum += src[k*rowStep+x];
        }
        
        y = 0;
        for (; y < (start_edge-3); y += 4) {
            // assert( (y - halfEdge) <= 0 );
            TS result0 = sum / edge;
            sum += src[((y+0)+remainEdge)*rowStep+x];
            sum -= src[0*rowStep+x];
            TS result1 = sum / edge;
            sum += src[((y+1)+remainEdge)*rowStep+x];
            sum -= src[0*rowStep+x];
            TS result2 = sum / edge;
            sum += src[((y+2)+remainEdge)*rowStep+x];
            sum -= src[0*rowStep+x];
            TS result3 = sum / edge;
            sum += src[((y+3)+remainEdge)*rowStep+x];
            sum -= src[0*rowStep+x];
            
            dest[(y+0)*rowStep+x] = T(result0);
            dest[(y+1)*rowStep+x] = T(result1);
            dest[(y+2)*rowStep+x] = T(result2);
            dest[(y+3)*rowStep+x] = T(result3);
        }
        for (; y < start_edge; ++y) {
            TS result = sum / edge;
            dest[y*rowStep+x] = T(result);
            
            // subtract old value, add new value
            // assert( (y - halfEdge) <= 0 );
            sum += src[(y+remainEdge)*rowStep+x];
            sum -= src[0*rowStep+x];
        }

        for (; y < (end_edge-3); y += 4) {
            TS result0 = sum / edge;
            sum += src[(y+0+remainEdge)*rowStep+x];
            sum -= src[(y+0-halfEdge)*rowStep+x];
            TS result1 = sum / edge;
            sum += src[(y+1+remainEdge)*rowStep+x];
            sum -= src[(y+1-halfEdge)*rowStep+x];
            TS result2 = sum / edge;
            sum += src[(y+2+remainEdge)*rowStep+x];
            sum -= src[(y+2-halfEdge)*rowStep+x];
            TS result3 = sum / edge;
            sum += src[(y+3+remainEdge)*rowStep+x];
            sum -= src[(y+3-halfEdge)*rowStep+x];
            
            dest[(y+0)*rowStep+x] = T(result0);
            dest[(y+1)*rowStep+x] = T(result1);
            dest[(y+2)*rowStep+x] = T(result2);
            dest[(y+3)*rowStep+x] = T(result3);
        }

        for (; y < end_edge; ++y) {
            TS result = sum / edge;
            dest[y*rowStep+x] = T(result);
            
            // subtract old value, add new value
            sum += src[(y+remainEdge)*rowStep+x];
            sum -= src[(y-halfEdge)*rowStep+x];
        }
        
        for (; y < (rows-4); y += 4) {
            TS result0 = sum / edge;
            sum += src[(rows-1)*rowStep+x];
            sum -= src[((y+0)-halfEdge)*rowStep+x];
            TS result1 = sum / edge;
            sum += src[(rows-1)*rowStep+x];
            sum -= src[((y+1)-halfEdge)*rowStep+x];
            TS result2 = sum / edge;
            sum += src[(rows-1)*rowStep+x];
            sum -= src[((y+2)-halfEdge)*rowStep+x];
            TS result3 = sum / edge;
            sum += src[(rows-1)*rowStep+x];
            sum -= src[((y+3)-halfEdge)*rowStep+x];
            
            dest[(y+0)*rowStep+x] = T(result0);
            dest[(y+1)*rowStep+x] = T(result1);
            dest[(y+2)*rowStep+x] = T(result2);
            dest[(y+3)*rowStep+x] = T(result3);
        }
        
        for (; y < (rows-1); ++y) {
            TS result = sum / edge;
            dest[y*rowStep+x] = T(result);
            
            // subtract old value, add new value
            sum += src[(rows-1)*rowStep+x];
            sum -= src[(y-halfEdge)*rowStep+x];
        }
        
        // write last pixel
        sum = sum / edge;
        dest[(rows-1)*rowStep+x] = T(sum);
    
    }
}

/******************************************************************************/

// Simple vertical box filter with edge replication
// a great way to thrash the cache and TLBs
// break out edge conditions into larger loops, only test edge conditions near edges
// stop doing redundant adds
// unroll in X to get better cache usage

// O(1)
template<typename T, typename TS>
void box_vert_opt5(const T *src, T *dest, int rows, int cols, int rowStep, int edge ) {
    const bool isFloat = (T(2.9) > T(2.0));
    const TS half = isFloat ? TS(0) : TS(edge/2);
    const int halfEdge = edge / 2;
    const int remainEdge = edge - halfEdge;
    const int start_edge = std::min(halfEdge,(rows-1));
    const int end_edge = (rows-1) - std::min(remainEdge,(rows-1));
    int x;
    
    // assert( rows >= edge );
    
    x = 0;

    for (; x < (cols-3); x += 4) {
        int y, k;
        TS sum[4] = { half, half, half, half };
        
        // sum for first pixel
        sum[0] += halfEdge * src[0*rowStep+x+0];
        sum[1] += halfEdge * src[0*rowStep+x+1];
        sum[2] += halfEdge * src[0*rowStep+x+2];
        sum[3] += halfEdge * src[0*rowStep+x+3];
        
        k = 0;
        for (; k < remainEdge; ++k) {
            sum[0] += src[k*rowStep+x+0];
            sum[1] += src[k*rowStep+x+1];
            sum[2] += src[k*rowStep+x+2];
            sum[3] += src[k*rowStep+x+3];
        }
        
        for (y = 0; y < start_edge; ++y) {
            TS result[4];
            result[0] = sum[0] / edge;
            result[1] = sum[1] / edge;
            result[2] = sum[2] / edge;
            result[3] = sum[3] / edge;
            
            // subtract old value, add new value
            // assert( (y - halfEdge) <= 0 );
            sum[0] += src[(y+remainEdge)*rowStep+x+0];
            sum[1] += src[(y+remainEdge)*rowStep+x+1];
            sum[2] += src[(y+remainEdge)*rowStep+x+2];
            sum[3] += src[(y+remainEdge)*rowStep+x+3];
            
            sum[0] -= src[0*rowStep+x+0];
            sum[1] -= src[0*rowStep+x+1];
            sum[2] -= src[0*rowStep+x+2];
            sum[3] -= src[0*rowStep+x+3];
            
            dest[y*rowStep+x+0] = T(result[0]);
            dest[y*rowStep+x+1] = T(result[1]);
            dest[y*rowStep+x+2] = T(result[2]);
            dest[y*rowStep+x+3] = T(result[3]);
        }

        for (; y < end_edge; ++y) {
            TS result[4];
            result[0] = sum[0] / edge;
            result[1] = sum[1] / edge;
            result[2] = sum[2] / edge;
            result[3] = sum[3] / edge;
            
            // subtract old value, add new value
            sum[0] += src[(y+remainEdge)*rowStep+x+0];
            sum[1] += src[(y+remainEdge)*rowStep+x+1];
            sum[2] += src[(y+remainEdge)*rowStep+x+2];
            sum[3] += src[(y+remainEdge)*rowStep+x+3];
            
            sum[0] -= src[(y-halfEdge)*rowStep+x+0];
            sum[1] -= src[(y-halfEdge)*rowStep+x+1];
            sum[2] -= src[(y-halfEdge)*rowStep+x+2];
            sum[3] -= src[(y-halfEdge)*rowStep+x+3];
            
            dest[y*rowStep+x+0] = T(result[0]);
            dest[y*rowStep+x+1] = T(result[1]);
            dest[y*rowStep+x+2] = T(result[2]);
            dest[y*rowStep+x+3] = T(result[3]);
        }
        
        for (; y < (rows-1); ++y) {
            TS result[4];
            result[0] = sum[0] / edge;
            result[1] = sum[1] / edge;
            result[2] = sum[2] / edge;
            result[3] = sum[3] / edge;
            
            // subtract old value, add new value
            sum[0] += src[(rows-1)*rowStep+x+0];
            sum[1] += src[(rows-1)*rowStep+x+1];
            sum[2] += src[(rows-1)*rowStep+x+2];
            sum[3] += src[(rows-1)*rowStep+x+3];
            
            sum[0] -= src[(y-halfEdge)*rowStep+x+0];
            sum[1] -= src[(y-halfEdge)*rowStep+x+1];
            sum[2] -= src[(y-halfEdge)*rowStep+x+2];
            sum[3] -= src[(y-halfEdge)*rowStep+x+3];
            
            dest[y*rowStep+x+0] = T(result[0]);
            dest[y*rowStep+x+1] = T(result[1]);
            dest[y*rowStep+x+2] = T(result[2]);
            dest[y*rowStep+x+3] = T(result[3]);
        }
        
        // write last pixels
            TS result[4];
            result[0] = sum[0] / edge;
            result[1] = sum[1] / edge;
            result[2] = sum[2] / edge;
            result[3] = sum[3] / edge;
        
            dest[y*rowStep+x+0] = T(result[0]);
            dest[y*rowStep+x+1] = T(result[1]);
            dest[y*rowStep+x+2] = T(result[2]);
            dest[y*rowStep+x+3] = T(result[3]);
    
    }
    
    for (; x < cols; ++x) {
        int y, k;
        TS sum = half;
        
        // sum for first pixel
        sum += halfEdge * src[0*rowStep+x];
        k = 0;
        for (; k < remainEdge; ++k) {
            sum += src[k*rowStep+x];
        }
        
        for (y = 0; y < start_edge; ++y) {
            TS result = sum / edge;
            dest[y*rowStep+x] = T(result);
            
            // subtract old value, add new value
            // assert( (y - halfEdge) <= 0 );
            sum += src[(y+remainEdge)*rowStep+x];
            sum -= src[0*rowStep+x];
        }

        for (; y < end_edge; ++y) {
            TS result = sum / edge;
            dest[y*rowStep+x] = T(result);
            
            // subtract old value, add new value
            sum += src[(y+remainEdge)*rowStep+x];
            sum -= src[(y-halfEdge)*rowStep+x];
        }
        
        for (; y < (rows-1); ++y) {
            TS result = sum / edge;
            dest[y*rowStep+x] = T(result);
            
            // subtract old value, add new value
            sum += src[(rows-1)*rowStep+x];
            sum -= src[(y-halfEdge)*rowStep+x];
        }
        
        // write last pixel
        sum = sum / edge;
        dest[(rows-1)*rowStep+x] = T(sum);
    
    }
}

/******************************************************************************/

// Simple vertical box filter with edge replication
// break out edge conditions into larger loops, only test edge conditions near edges
// stop doing redundant adds
// reorder loops and iterate over a stack temporary array to get much better cache usage

// O(1)
template<typename T, typename TS>
void box_vert_opt6(const T *src, T *dest, int rows, int cols, int rowStep, int edge ) {
    const bool isFloat = (T(2.9) > T(2.0));
    const TS half = isFloat ? TS(0) : TS(edge/2);
    const int halfEdge = edge / 2;
    const int remainEdge = edge - halfEdge;
    const int start_edge = std::min(halfEdge,(rows-1));
    const int end_edge = (rows-1) - std::min(remainEdge,(rows-1));
    const int bufferSize = 256;
    TS buffer[ bufferSize ];
    int xx;
    
    // assert( rows >= edge );

    for (xx = 0; xx < cols; xx += bufferSize) {
        int y, x, i, k;
        int endx = xx + bufferSize;
        if (endx > cols)
            endx = cols;

        for (x = xx, i = 0; x < endx; ++x, ++i)
            buffer[i] = half + (halfEdge * src[0*rowStep+x]);
        
        k = 0;
        for (; k < remainEdge; ++k)
            for (x = xx, i = 0; x < endx; ++x, ++i)
                buffer[i] += src[k*rowStep+x];
        
        for (y = 0; y < start_edge; ++y) {
        
            for (x = xx, i = 0; x < endx; ++x, ++i) {
                TS result = buffer[i] / edge;
                dest[y*rowStep+x] = T(result);
            }
            
            // subtract old value, add new value
            // assert( (y - halfEdge) <= 0 );
            for (x = xx, i = 0; x < endx; ++x, ++i) {
                buffer[i] += src[(y+remainEdge)*rowStep+x];
                buffer[i] -= src[0*rowStep+x];
            }
        }

        for (; y < end_edge; ++y) {
        
            for (x = xx, i = 0; x < endx; ++x, ++i) {
                TS result = buffer[i] / edge;
                dest[y*rowStep+x] = T(result);
            }
            
            // subtract old value, add new value
            for (x = xx, i = 0; x < endx; ++x, ++i) {
                buffer[i] += src[(y+remainEdge)*rowStep+x];
                buffer[i] -= src[(y-halfEdge)*rowStep+x];
            }
        }
    
        for (; y < (rows-1); ++y) {
        
            for (x = xx, i = 0; x < endx; ++x, ++i) {
                TS result = buffer[i] / edge;
                dest[y*rowStep+x] = T(result);
            }
            
            // subtract old value, add new value
            for (x = xx, i = 0; x < endx; ++x, ++i) {
                buffer[i] += src[(rows-1)*rowStep+x];
                buffer[i] -= src[(y-halfEdge)*rowStep+x];
            }
        }
    
        // write last row of pixels
        for (x = xx, i = 0; x < endx; ++x, ++i) {
            TS result = buffer[i] / edge;
            dest[(rows-1)*rowStep+x] = T(result);
        }

    }   // xx
}

/******************************************************************************/

// Simple vertical box filter with edge replication
// break out edge conditions into larger loops, only test edge conditions near edges
// stop doing redundant adds
// swap loops and iterate over a stack temporary array to get much better cache usage
// unroll some inner loops

// O(1)
template<typename T, typename TS>
void box_vert_opt7(const T *src, T *dest, int rows, int cols, int rowStep, int edge ) {
    const bool isFloat = (T(2.9) > T(2.0));
    const TS half = isFloat ? TS(0) : TS(edge/2);
    const int halfEdge = edge / 2;
    const int remainEdge = edge - halfEdge;
    const int start_edge = std::min(halfEdge,(rows-1));
    const int end_edge = (rows-1) - std::min(remainEdge,(rows-1));
    const int bufferSize = 256;
    TS buffer[ bufferSize ];
    int xx;
    
    // assert( rows >= edge );

    for (xx = 0; xx < cols; xx += bufferSize) {
        int y, x, i, k;
        int endx = xx + bufferSize;
        if (endx > cols)
            endx = cols;

        x = xx; i = 0;
        
        for (; x < (endx-3); x += 4, i += 4) {
            buffer[i+0] = half + halfEdge * src[0*rowStep+x+0];
            buffer[i+1] = half + halfEdge * src[0*rowStep+x+1];
            buffer[i+2] = half + halfEdge * src[0*rowStep+x+2];
            buffer[i+3] = half + halfEdge * src[0*rowStep+x+3];
        }

        for (; x < endx; ++x, ++i)
            buffer[i] = half + halfEdge * src[0*rowStep+x];
        
        k = 0;

        for (; k < (remainEdge-1); k += 2)
            for (x = xx, i = 0; x < endx; ++x, ++i)
                buffer[i] += src[(k+0)*rowStep+x]
                          + src[(k+1)*rowStep+x];

        for (; k < remainEdge; ++k)
            for (x = xx, i = 0; x < endx; ++x, ++i)
                buffer[i] += src[k*rowStep+x];
        
        for (y = 0; y < start_edge; ++y) {
        
            // assert( (y - halfEdge) <= 0 );
            for (x = xx, i = 0; x < endx; ++x, ++i) {
                TS result = buffer[i] / edge;
                dest[y*rowStep+x] = T(result);
                
                buffer[i] += src[(y+remainEdge)*rowStep+x]
                        - src[0*rowStep+x];
            }
        }

        for (; y < end_edge; ++y) {
            T *destRow = dest + y*rowStep;
        
            x = xx; i = 0;

            for (; x < (endx-3); x += 4, i += 4) {
                TS result[4];
                result[0] = buffer[i+0] / edge;
                result[1] = buffer[i+1] / edge;
                result[2] = buffer[i+2] / edge;
                result[3] = buffer[i+3] / edge;
                
                destRow[x+0] = T(result[0]);
                destRow[x+1] = T(result[1]);
                destRow[x+2] = T(result[2]);
                destRow[x+3] = T(result[3]);
                
                // subtract old value, add new value
                buffer[i+0] += src[(y+remainEdge)*rowStep+x+0]
                            - src[(y-halfEdge)*rowStep+x+0];
                buffer[i+1] += src[(y+remainEdge)*rowStep+x+1]
                            - src[(y-halfEdge)*rowStep+x+1];
                buffer[i+2] += src[(y+remainEdge)*rowStep+x+2]
                            - src[(y-halfEdge)*rowStep+x+2];
                buffer[i+3] += src[(y+remainEdge)*rowStep+x+3]
                            - src[(y-halfEdge)*rowStep+x+3];
            }

// NOTE - LLVM does better with tight loop than unrolled, seems to get confused
            for (; x < endx; ++x, ++i) {
                TS result = buffer[i] / edge;
                
                dest[y*rowStep+x] = T(result);
                
                buffer[i] += src[(y+remainEdge)*rowStep+x]
                        - src[(y-halfEdge)*rowStep+x];
            }
            
        }
    
        for (; y < (rows-1); ++y) {
        
            for (x = xx, i = 0; x < endx; ++x, ++i) {
                TS result = buffer[i] / edge;
                
                dest[y*rowStep+x] = T(result);
                
                buffer[i] += src[(rows-1)*rowStep+x]
                        - src[(y-halfEdge)*rowStep+x];
            }
        }
    
        // write last row of pixels
        for (x = xx, i = 0; x < endx; ++x, ++i) {
            TS result = buffer[i] / edge;
            dest[(rows-1)*rowStep+x] = T(result);
        }

    }   // xx block looop
}

/******************************************************************************/
/******************************************************************************/

// Simple vertical box filter without edge conditions (prepadded buffer)
// a great way to thrash the cache and TLBs

// O(N)
template<typename T, typename TS>
void box_vert_pad(const T *src, T *dest, int rows, int cols, int rowStep, int edge ) {
    const bool isFloat = (T(2.9) > T(2.0));
    const TS half = isFloat ? TS(0) : TS(edge/2);
    
    for (int x = 0; x < cols; ++x) {
        for (int y = 0; y < rows; ++y) {
            TS sum = half;
            
            for (int k = 0; k < edge; ++k) {
                sum += src[(y+k)*rowStep+x];
            }
            
            sum = sum / edge;
            
            dest[y*rowStep+x] = T(sum);
        }
    }
}

/******************************************************************************/

// Simple vertical box filter without edge conditions (prepadded buffer)
// a great way to thrash the cache and TLBs
// unroll obvious, but wrong inner loop

// O(N)
template<typename T, typename TS>
void box_vert_pad_opt1(const T *src, T *dest, int rows, int cols, int rowStep, int edge ) {
    const bool isFloat = (T(2.9) > T(2.0));
    const TS half = isFloat ? TS(0) : TS(edge/2);
    
    for (int x = 0; x < cols; ++x) {
        for (int y = 0; y < rows; ++y) {
            TS sum = half;
            int k;
            
            k = 0;
            for (; k < (edge-4); k += 4) {
                sum += src[(y+k+0)*rowStep+x];
                sum += src[(y+k+1)*rowStep+x];
                sum += src[(y+k+2)*rowStep+x];
                sum += src[(y+k+3)*rowStep+x];
            }
            for (; k < edge; ++k) {
                sum += src[(y+k)*rowStep+x];
            }
            
            sum = sum / edge;
            
            dest[y*rowStep+x] = T(sum);
        }
    }
}

/******************************************************************************/

// Simple vertical box filter without edge conditions (prepadded buffer)
// unroll in X to get better cache usage

// O(N)
template<typename T, typename TS>
void box_vert_pad_opt2(const T *src, T *dest, int rows, int cols, int rowStep, int edge ) {
    const bool isFloat = (T(2.9) > T(2.0));
    const TS half = isFloat ? TS(0) : TS(edge/2);
    int x;
    
    x = 0;

    for (; x < (cols-3); x += 4) {
        for (int y = 0; y < rows; ++y) {
            TS sum[4] = { half, half, half, half };
            
            for (int k = 0; k < edge; ++k) {
                sum[0] += src[(y+k)*rowStep+x+0];
                sum[1] += src[(y+k)*rowStep+x+1];
                sum[2] += src[(y+k)*rowStep+x+2];
                sum[3] += src[(y+k)*rowStep+x+3];
            }
            
            sum[0] = sum[0] / edge;
            sum[1] = sum[1] / edge;
            sum[2] = sum[2] / edge;
            sum[3] = sum[3] / edge;
            
            dest[y*rowStep+x+0] = T(sum[0]);
            dest[y*rowStep+x+1] = T(sum[1]);
            dest[y*rowStep+x+2] = T(sum[2]);
            dest[y*rowStep+x+3] = T(sum[3]);
        }
    }

    for (; x < cols; ++x) {
        for (int y = 0; y < rows; ++y) {
            TS sum = half;
            
            for (int k = 0; k < edge; ++k) {
                sum += src[(y+k)*rowStep+x];
            }
            
            sum = sum / edge;
            
            dest[y*rowStep+x] = T(sum);
        }
    }
}

/******************************************************************************/

// Simple vertical box filter without edge conditions (prepadded buffer)
// stop doing redundant adds

// O(1)
template<typename T, typename TS>
void box_vert_pad_opt3(const T *src, T *dest, int rows, int cols, int rowStep, int edge ) {
    const bool isFloat = (T(2.9) > T(2.0));
    const TS half = isFloat ? TS(0) : TS(edge/2);
    int x;
    
    for (x = 0; x < cols; ++x) {
        TS sum = half;
    
        for (int k = 0; k < edge; ++k) {
            sum += src[k*rowStep+x];
        }
        
        for (int y = 0; y < (rows-1); ++y) {
            TS result = sum / edge;
            dest[y*rowStep+x] = T(result);
            
            sum += src[(y+edge)*rowStep+x];
            sum -= src[(y)*rowStep+x];
        }
        
        TS result = sum / edge;
        dest[(rows-1)*rowStep+x] = T(result);
    }
}

/******************************************************************************/

// Simple vertical box filter without edge conditions (prepadded buffer)
// stop doing redundant adds
// unroll in X to get better cache usage

// O(1)
template<typename T, typename TS>
void box_vert_pad_opt4(const T *src, T *dest, int rows, int cols, int rowStep, int edge ) {
    const bool isFloat = (T(2.9) > T(2.0));
    const TS half = isFloat ? TS(0) : TS(edge/2);
    int x;
    
    x = 0;

    for (; x < (cols-3); x += 4) {
        int y;

        TS sum[4] = { half, half, half, half };
    
        for (int k = 0; k < edge; ++k) {
            sum[0] += src[k*rowStep+x+0];
            sum[1] += src[k*rowStep+x+1];
            sum[2] += src[k*rowStep+x+2];
            sum[3] += src[k*rowStep+x+3];
        }
    
        for (y = 0; y < (rows-1); ++y) {
            TS result[4];
            result[0] = sum[0] / edge;
            result[1] = sum[1] / edge;
            result[2] = sum[2] / edge;
            result[3] = sum[3] / edge;
            
            dest[y*rowStep+x+0] = T(result[0]);
            dest[y*rowStep+x+1] = T(result[1]);
            dest[y*rowStep+x+2] = T(result[2]);
            dest[y*rowStep+x+3] = T(result[3]);
            
            // subtract old value, add new value
            sum[0] += src[(y+edge)*rowStep+x+0];
            sum[1] += src[(y+edge)*rowStep+x+1];
            sum[2] += src[(y+edge)*rowStep+x+2];
            sum[3] += src[(y+edge)*rowStep+x+3];
            
            sum[0] -= src[(y)*rowStep+x+0];
            sum[1] -= src[(y)*rowStep+x+1];
            sum[2] -= src[(y)*rowStep+x+2];
            sum[3] -= src[(y)*rowStep+x+3];
        }
        
            y = (rows-1);
            TS result[4];
            result[0] = sum[0] / edge;
            result[1] = sum[1] / edge;
            result[2] = sum[2] / edge;
            result[3] = sum[3] / edge;
        
            dest[y*rowStep+x+0] = T(result[0]);
            dest[y*rowStep+x+1] = T(result[1]);
            dest[y*rowStep+x+2] = T(result[2]);
            dest[y*rowStep+x+3] = T(result[3]);
    }

    for (; x < cols; ++x) {
        TS sum = half;
    
        for (int k = 0; k < edge; ++k) {
            sum += src[k*rowStep+x];
        }
        
        for (int y = 0; y < (rows-1); ++y) {
            TS result = sum / edge;
            dest[y*rowStep+x] = T(result);
            
            sum += src[(y+edge)*rowStep+x];
            sum -= src[(y)*rowStep+x];
        }
        
            TS result = sum / edge;
            dest[(rows-1)*rowStep+x] = T(result);
    }
}

/******************************************************************************/

// Simple vertical box filter without edge conditions (prepadded buffer)
// stop doing redundant adds
// reorder loops and iterate over a stack temporary array to get much better cache usage

// O(1)
template<typename T, typename TS>
void box_vert_pad_opt5(const T *src, T *dest, int rows, int cols, int rowStep, int edge ) {
    const bool isFloat = (T(2.9) > T(2.0));
    const TS half = isFloat ? TS(0) : TS(edge/2);
    const int bufferSize = 256;
    TS buffer[ bufferSize ];
    int xx;

    for (xx = 0; xx < cols; xx += bufferSize) {
        int y, x, i, k;
        int endx = xx + bufferSize;
        if (endx > cols)
            endx = cols;

        // sum initial rows into temporary buffer
        k = 0;
            for (x = xx, i = 0; x < endx; ++x, ++i)
                buffer[i] = half + src[0*rowStep+x];
        
        k = 1;
        for (; k < edge; ++k)
            for (x = xx, i = 0; x < endx; ++x, ++i)
                buffer[i] += src[k*rowStep+x];

        for (y = 0; y < (rows-1); ++y) {
            for (x = xx, i = 0; x < endx; ++x, ++i) {
                TS result = buffer[i] / edge;
                
                buffer[i] += src[(y+edge)*rowStep+x];
                buffer[i] -= src[(y)*rowStep+x];
                
                dest[y*rowStep+x] = T(result);
            }
        }
    
        // write last row of pixels
        for (x = xx, i = 0; x < endx; ++x, ++i) {
            TS result = buffer[i] / edge;
            dest[(rows-1)*rowStep+x] = T(result);
        }

    }   // xx
}

/******************************************************************************/

// Simple vertical box filter without edge conditions (prepadded buffer)
// stop doing redundant adds
// reorder loops and iterate over a stack temporary array to get much better cache usage
// unroll some inner loops - helps some compilers, and confuses others

template<typename T, typename TS>
void box_vert_pad_opt6(const T *src, T *dest, int rows, int cols, int rowStep, int edge ) {
    const bool isFloat = (T(2.9) > T(2.0));
    const TS half = isFloat ? TS(0) : TS(edge/2);
    const int bufferSize = 256;
    TS buffer[ bufferSize ];
    int xx;

    for (xx = 0; xx < cols; xx += bufferSize) {
        int y, x, i, k;
        int endx = xx + bufferSize;
        if (endx > cols)
            endx = cols;

        // sum initial rows into temporary buffer
        k = 0;
        x = xx; i = 0;

            for (; x < (endx-3); x += 4, i += 4) {
                buffer[i+0] = half + src[0*rowStep+x+0];
                buffer[i+1] = half + src[0*rowStep+x+1];
                buffer[i+2] = half + src[0*rowStep+x+2];
                buffer[i+3] = half + src[0*rowStep+x+3];
            }

            for (; x < endx; ++x, ++i)
                buffer[i] = half + src[0*rowStep+x];
        
        k = 1;

        for (; k < (edge-1); k += 2) {
            x = xx; i = 0;
            for (; x < (endx-3); x += 4, i += 4) {
                buffer[i+0] += src[(k+0)*rowStep+x+0]
                            + src[(k+1)*rowStep+x+0];
                buffer[i+1] += src[(k+0)*rowStep+x+1]
                            + src[(k+1)*rowStep+x+1];
                buffer[i+2] += src[(k+0)*rowStep+x+2]
                            + src[(k+1)*rowStep+x+2];
                buffer[i+3] += src[(k+0)*rowStep+x+3]
                            + src[(k+1)*rowStep+x+3];
            }
            for (; x < endx; ++x, ++i)
                buffer[i] += src[(k+0)*rowStep+x]
                          + src[(k+1)*rowStep+x];
        }

        for (; k < edge; ++k)
            for (x = xx, i = 0; x < endx; ++x, ++i)
                buffer[i] += src[k*rowStep+x];

        for (y = 0; y < (rows-1); ++y) {
            T *destRow = dest + y*rowStep;
            
            x = xx; i = 0;

            for (; x < (endx-3); x += 4, i += 4) {
                TS result[4];
                result[0] = buffer[i+0] / edge;
                result[1] = buffer[i+1] / edge;
                result[2] = buffer[i+2] / edge;
                result[3] = buffer[i+3] / edge;
                
                buffer[i+0] += src[(y+edge)*rowStep+x+0]
                            - src[(y)*rowStep+x+0];
                buffer[i+1] += src[(y+edge)*rowStep+x+1]
                            - src[(y)*rowStep+x+1];
                buffer[i+2] += src[(y+edge)*rowStep+x+2]
                            - src[(y)*rowStep+x+2];
                buffer[i+3] += src[(y+edge)*rowStep+x+3]
                            - src[(y)*rowStep+x+3];
                
                destRow[x+0] = T(result[0]);
                destRow[x+1] = T(result[1]);
                destRow[x+2] = T(result[2]);
                destRow[x+3] = T(result[3]);
            }

            for (; x < endx; ++x, ++i) {
                TS result = buffer[i] / edge;
                
                destRow[x] = T(result);
                
                buffer[i] += src[(y+edge)*rowStep+x]
                        - src[(y)*rowStep+x];
            }
        }
    
        // write last row of pixels
        x = xx; i = 0;

        for (; x < (endx-3); x += 4, i += 4) {
            TS result[4];
            result[0] = buffer[i+0] / edge;
            result[1] = buffer[i+1] / edge;
            result[2] = buffer[i+2] / edge;
            result[3] = buffer[i+3] / edge;
        
            dest[(rows-1)*rowStep+x+0] = T(result[0]);
            dest[(rows-1)*rowStep+x+1] = T(result[1]);
            dest[(rows-1)*rowStep+x+2] = T(result[2]);
            dest[(rows-1)*rowStep+x+3] = T(result[3]);
        }

        for (; x < endx; ++x, ++i) {
            TS result = buffer[i] / edge;
            dest[(rows-1)*rowStep+x] = T(result);
        }

    }   // xx
}

/******************************************************************************/
/******************************************************************************/

// Simple 2D box filter without edge conditions (prepadded buffer)
// NOTE - this should use larger accumulator than vert or horiz, because (edge*edge*values+edge/2)
// A great way to thrash the cache and TLBs

// O(N^2)
template<typename T, typename TS>
void box_2D_pad(const T *src, T *dest, int rows, int cols, int rowStep, int edge ) {
    const bool isFloat = (T(2.9) > T(2.0));
    const TS edge2 = edge*edge;
    const TS half = isFloat ? TS(0) : TS(edge2/2);
    
    for (int y = 0; y < rows; ++y) {
        T *destRow = dest + y*rowStep;
        
        for (int x = 0; x < cols; ++x) {
            TS sum = half;
            
            for (int ky = 0; ky < edge; ++ky)
                for (int kx = 0; kx < edge; ++kx)
                    sum += src[(y+ky)*rowStep+(x+kx)];
            
            sum = sum / edge2;
            
            destRow[x] = T(sum);
        }
    }
}

/******************************************************************************/

// Simple 2D box filter without edge conditions (prepadded buffer)
// NOTE - this should use larger accumulator than vert or horiz, because (edge*edge*values+edge/2)
// stop doing quite so many redundant adds

// O(N)
template<typename T, typename TS>
void box_2D_pad_opt1(const T *src, T *dest, int rows, int cols, int rowStep, int edge ) {
    const bool isFloat = (T(2.9) > T(2.0));
    const TS edge2 = edge*edge;
    const TS half = isFloat ? TS(0) : TS(edge2/2);
    
    for (int y = 0; y < rows; ++y) {
        T *destRow = dest + y*rowStep;
        TS sum = half;
    
        // sum up first pixel
            for (int ky = 0; ky < edge; ++ky)
                for (int kx = 0; kx < edge; ++kx)
                    sum += src[(y+ky)*rowStep+(kx)];
        
        for (int x = 0; x < (cols-1); ++x) {
            TS result = sum / edge2;
            destRow[x] = T(result);
            
            for (int ky = 0; ky < edge; ++ky)
                sum += src[(y+ky)*rowStep+(x+edge)]
                    - src[(y+ky)*rowStep+(x)];
        }
        
        // output last pixel
            sum = sum / edge2;
            destRow[(cols-1)] = T(sum);
        
    }
}

/******************************************************************************/

// Simple 2D box filter without edge conditions (prepadded buffer)
// NOTE - this should use larger accumulator than vert or horiz, because (edge*edge*values+edge/2)
// stop doing redundant adds
// reorder loops and iterate over a stack temporary array to get much better cache usage

// O(1)
template<typename T, typename TS>
void box_2D_pad_opt2(const T *src, T *dest, int rows, int cols, int rowStep, int edge ) {
    const bool isFloat = (T(2.9) > T(2.0));
    const TS edge2 = edge*edge;
    const TS half = isFloat ? TS(0) : TS(edge2/2);
    const int bufferSize = 512;
    TS buffer[ bufferSize ];
    const int blockSize = bufferSize - edge;    // allow for horizontal padding
    int xx;

    for (xx = 0; xx < cols; xx += blockSize) {
        int y, x, i, k;
        int endx = xx + blockSize;
        if (endx > cols)
            endx = cols;

        // sum initial rows into temporary column buffer
        k = 0;
            for (x = xx, i = 0; x < (endx+edge); ++x, ++i)
                buffer[i] = src[0*rowStep+x];
        
        k = 1;
        for (; k < edge; ++k)
            for (x = xx, i = 0; x < (endx+edge); ++x, ++i)
                buffer[i] += src[k*rowStep+x];
    
        for (y = 0; y < rows; ++y) {
            T *destRow = dest + y*rowStep;
            TS sum = half;
        
            // sum up first pixel
                for (int kx = 0; kx < edge; ++kx)
                    sum += buffer[kx];
            
            for (x = xx, i = 0; x < (endx-1); ++x, ++i) {
                TS result = sum / edge2;
                destRow[x] = T(result);
                
                sum += buffer[i+edge]
                     - buffer[i];
            }
            
            // output last pixel for this row
                TS result = sum / edge2;
                destRow[(endx-1)] = T(result);
        
            // update column buffer
            if (y < (rows-1))
                for (x = xx, i = 0; x < (endx+edge); ++x, ++i)
                    buffer[i] += src[(y+edge)*rowStep+x]
                               - src[y*rowStep+x];
        }   // y
    
    }   // xx
}

/******************************************************************************/

// Simple 2D box filter without edge conditions (prepadded buffer)
// NOTE - this should use larger accumulator than vert or horiz, because (edge*edge*values+edge/2)
// stop doing redundant adds
// reorder loops and iterate over a stack temporary array to get much better cache usage
// unroll some loops - helps some compilers, and confuses others

// O(1)
template<typename T, typename TS>
void box_2D_pad_opt3(const T *src, T *dest, int rows, int cols, int rowStep, int edge ) {
    const bool isFloat = (T(2.9) > T(2.0));
    const TS edge2 = edge*edge;
    const TS half = isFloat ? TS(0) : TS(edge2/2);
    const int bufferSize = 512;     // 768-800 seems to be good spot for float
    TS buffer[ bufferSize ];
    const int blockSize = bufferSize - edge;    // allow for horizontal padding
    int xx;
    
    // assert( bufferSize > edge );
    
    for (xx = 0; xx < cols; xx += blockSize) {
        int y, x, i, k;
        int endx = xx + blockSize;
        if (endx > cols)
            endx = cols;

        // sum initial rows into temporary column buffer
        k = 0;

            std::copy_n( src+xx, (endx+edge-xx), buffer );

        k = 1;

        for (; k < (edge-1); k += 2) {
            x = xx; i = 0;
            for (; x < (endx+edge-3); x += 4, i += 4) {
                buffer[i+0] += src[(k+0)*rowStep+x+0]
                            + src[(k+1)*rowStep+x+0];
                buffer[i+1] += src[(k+0)*rowStep+x+1]
                            + src[(k+1)*rowStep+x+1];
                buffer[i+2] += src[(k+0)*rowStep+x+2]
                            + src[(k+1)*rowStep+x+2];
                buffer[i+3] += src[(k+0)*rowStep+x+3]
                            + src[(k+1)*rowStep+x+3];
            }
            for (; x < (endx+edge); ++x, ++i)
                buffer[i] += src[(k+0)*rowStep+x]
                        + src[(k+1)*rowStep+x];
        }

        for (; k < edge; ++k) {
            x = xx; i = 0;

            for (; x < (endx+edge-3); x += 4, i += 4) {
                buffer[i+0] += src[k*rowStep+x+0];
                buffer[i+1] += src[k*rowStep+x+1];
                buffer[i+2] += src[k*rowStep+x+2];
                buffer[i+3] += src[k*rowStep+x+3];
            }

            for (; x < (endx+edge); ++x, ++i)
                buffer[i] += src[k*rowStep+x];
        }
    
        for (y = 0; y < rows; ++y) {
            T *destRow = dest + y*rowStep;
            TS sum = half;
        
            // sum up first pixel
                for (int kx = 0; kx < edge; ++kx)
                    sum += buffer[kx];
            
            x = xx; i = 0;

            for (; x < (endx-4); x += 4, i += 4) {
                TS result[4];
                
                result[0] = sum;
                sum += buffer[i+0+edge]
                     - buffer[i+0];
                
                result[1] = sum;
                sum += buffer[i+1+edge]
                     - buffer[i+1];
                
                result[2] = sum;
                sum += buffer[i+2+edge]
                     - buffer[i+2];
                
                result[3] = sum;
                sum += buffer[i+3+edge]
                     - buffer[i+3];
                
                result[0] /= edge2;
                result[1] /= edge2;
                result[2] /= edge2;
                result[3] /= edge2;
                
                destRow[x+0] = T(result[0]);
                destRow[x+1] = T(result[1]);
                destRow[x+2] = T(result[2]);
                destRow[x+3] = T(result[3]);
            }

            for (; x < (endx-1); ++x, ++i) {
                TS result = sum / edge2;
                destRow[x] = T(result);
                
                sum += buffer[i+edge]
                     - buffer[i];
            }
            
            // output last pixel for this row
                TS result = sum / edge2;
                destRow[(endx-1)] = T(result);
        
            // update column buffer
            if (y < (rows-1)) {
                x = xx; i = 0;

                for (; x < (endx-3); x += 4, i += 4) {
                    buffer[i+0] += src[(y+edge)*rowStep+x+0]
                               - src[y*rowStep+x+0];
                    buffer[i+1] += src[(y+edge)*rowStep+x+1]
                               - src[y*rowStep+x+1];
                    buffer[i+2] += src[(y+edge)*rowStep+x+2]
                               - src[y*rowStep+x+2];
                    buffer[i+3] += src[(y+edge)*rowStep+x+3]
                               - src[y*rowStep+x+3];
                }

                for (; x < (endx+edge); ++x, ++i)
                    buffer[i] += src[(y+edge)*rowStep+x]
                               - src[y*rowStep+x];
            }
        
        }   // y
    
    }   // xx
}

/******************************************************************************/
/******************************************************************************/

template <typename T, typename Summer>
void test_conv(const T *src, T *dest, int rows, int cols, int rowStep, int edge, Summer func, const std::string label) {

    std::fill_n( dest, SIZE, T(init_value+2) );
    
    start_timer();

    for(int i = 0; i < iterations; ++i) {
        func( src, dest, rows, cols, rowStep, edge );
    }
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
    
    check_add( dest, rows, cols, rowStep, label);
}

/******************************************************************************/
/******************************************************************************/

template< typename T, typename TS >
void TestOneType()
{
    const int base_iterations = iterations;
    int edge = 21;

    std::string myTypeName( getTypeName<T>() );
    std::string myTypeNameA( getTypeName<TS>() );
    
    gLabels.clear();

    // work around MSVC bugs
    std::unique_ptr<T> data_flat_unique( new T[ HEIGHT*WIDTH ] );
    std::unique_ptr<T> data_flatDst_unique( new T[ HEIGHT*WIDTH ] );
    T *data_flatSrc = data_flat_unique.get();
    T *data_flatDst = data_flatDst_unique.get();


    std::fill_n( data_flatSrc, SIZE, T(init_value) );



    test_conv( data_flatSrc, data_flatDst, 1, SIZE, SIZE, edge, box_horiz<T,TS>, myTypeName + " box horiz 1D");
    test_conv( data_flatSrc, data_flatDst, 1, SIZE, SIZE, edge, box_horiz_opt1<T,TS>, myTypeName + " box horiz 1D opt1");
    test_conv( data_flatSrc, data_flatDst, 1, SIZE, SIZE, edge, box_horiz_opt2<T,TS>, myTypeName + " box horiz 1D opt2");
    test_conv( data_flatSrc, data_flatDst, 1, SIZE, SIZE, edge, box_horiz_opt3<T,TS>, myTypeName + " box horiz 1D opt3");
    test_conv( data_flatSrc, data_flatDst, 1, SIZE, SIZE, edge, box_horiz_opt4<T,TS>, myTypeName + " box horiz 1D opt4");
    
    std::string temp1( myTypeName + " convolution_box Horizontal 1D" );
    summarize( temp1.c_str(), SIZE, iterations, kDontShowGMeans, kDontShowPenalty );


    test_conv( data_flatSrc, data_flatDst, HEIGHT, WIDTH, WIDTH, edge, box_horiz<T,TS>, myTypeName + " box horiz 2D");
    test_conv( data_flatSrc, data_flatDst, HEIGHT, WIDTH, WIDTH, edge, box_horiz_opt1<T,TS>, myTypeName + " box horiz 2D opt1");
    test_conv( data_flatSrc, data_flatDst, HEIGHT, WIDTH, WIDTH, edge, box_horiz_opt2<T,TS>, myTypeName + " box horiz 2D opt2");
    test_conv( data_flatSrc, data_flatDst, HEIGHT, WIDTH, WIDTH, edge, box_horiz_opt3<T,TS>, myTypeName + " box horiz 2D opt3");
    test_conv( data_flatSrc, data_flatDst, HEIGHT, WIDTH, WIDTH, edge, box_horiz_opt4<T,TS>, myTypeName + " box horiz 2D opt4");
    
    std::string temp2( myTypeName + " convolution_box Horizontal 2D" );
    summarize( temp2.c_str(), SIZE, iterations, kDontShowGMeans, kDontShowPenalty );
    
    
    
    test_conv( data_flatSrc, data_flatDst, 1, SIZE-edge, SIZE-edge, edge, box_horiz_pad<T,TS>, myTypeName + " box horiz 1D padded");
    test_conv( data_flatSrc, data_flatDst, 1, SIZE-edge, SIZE-edge, edge, box_horiz_pad_opt1<T,TS>, myTypeName + " box horiz 1D padded opt1");
    test_conv( data_flatSrc, data_flatDst, 1, SIZE-edge, SIZE-edge, edge, box_horiz_pad_opt2<T,TS>, myTypeName + " box horiz 1D padded opt2");
    test_conv( data_flatSrc, data_flatDst, 1, SIZE-edge, SIZE-edge, edge, box_horiz_pad_opt3<T,TS>, myTypeName + " box horiz 1D padded opt3");
    test_conv( data_flatSrc, data_flatDst, 1, SIZE-edge, SIZE-edge, edge, box_horiz_pad_opt4<T,TS>, myTypeName + " box horiz 1D padded opt4");
    
    std::string temp3( myTypeName + " convolution_box Horizontal 1D padded" );
    summarize( temp3.c_str(), (SIZE-edge), iterations, kDontShowGMeans, kDontShowPenalty );


    test_conv( data_flatSrc, data_flatDst, HEIGHT, WIDTH-edge, WIDTH, edge, box_horiz_pad<T,TS>, myTypeName + " box horiz 2D padded");
    test_conv( data_flatSrc, data_flatDst, HEIGHT, WIDTH-edge, WIDTH, edge, box_horiz_pad_opt1<T,TS>, myTypeName + " box horiz 2D padded opt1");
    test_conv( data_flatSrc, data_flatDst, HEIGHT, WIDTH-edge, WIDTH, edge, box_horiz_pad_opt2<T,TS>, myTypeName + " box horiz 2D padded opt2");
    test_conv( data_flatSrc, data_flatDst, HEIGHT, WIDTH-edge, WIDTH, edge, box_horiz_pad_opt3<T,TS>, myTypeName + " box horiz 2D padded opt3");
    test_conv( data_flatSrc, data_flatDst, HEIGHT, WIDTH-edge, WIDTH, edge, box_horiz_pad_opt4<T,TS>, myTypeName + " box horiz 2D padded opt4");
    
    std::string temp4( myTypeName + " convolution_box Horizontal 2D padded" );
    summarize( temp4.c_str(), (HEIGHT*(WIDTH-edge)), iterations, kDontShowGMeans, kDontShowPenalty );
    



    test_conv( data_flatSrc, data_flatDst, HEIGHT, WIDTH, WIDTH, edge, box_vert<T,TS>, myTypeName + " box vert 2D");
    test_conv( data_flatSrc, data_flatDst, HEIGHT, WIDTH, WIDTH, edge, box_vert_opt1<T,TS>, myTypeName + " box vert 2D opt1");
    test_conv( data_flatSrc, data_flatDst, HEIGHT, WIDTH, WIDTH, edge, box_vert_opt2<T,TS>, myTypeName + " box vert 2D opt2");
    test_conv( data_flatSrc, data_flatDst, HEIGHT, WIDTH, WIDTH, edge, box_vert_opt3<T,TS>, myTypeName + " box vert 2D opt3");
    test_conv( data_flatSrc, data_flatDst, HEIGHT, WIDTH, WIDTH, edge, box_vert_opt4<T,TS>, myTypeName + " box vert 2D opt4");
    test_conv( data_flatSrc, data_flatDst, HEIGHT, WIDTH, WIDTH, edge, box_vert_opt5<T,TS>, myTypeName + " box vert 2D opt5");
    test_conv( data_flatSrc, data_flatDst, HEIGHT, WIDTH, WIDTH, edge, box_vert_opt6<T,TS>, myTypeName + " box vert 2D opt6");
    test_conv( data_flatSrc, data_flatDst, HEIGHT, WIDTH, WIDTH, edge, box_vert_opt7<T,TS>, myTypeName + " box vert 2D opt7");
    
    std::string temp5( myTypeName + " convolution_box Vertical 2D" );
    summarize( temp5.c_str(), SIZE, iterations, kDontShowGMeans, kDontShowPenalty );



    test_conv( data_flatSrc, data_flatDst, HEIGHT-edge, WIDTH, WIDTH, edge, box_vert_pad<T,TS>, myTypeName + " box vert 2D padded");
    test_conv( data_flatSrc, data_flatDst, HEIGHT-edge, WIDTH, WIDTH, edge, box_vert_pad_opt1<T,TS>, myTypeName + " box vert 2D padded opt1");
    test_conv( data_flatSrc, data_flatDst, HEIGHT-edge, WIDTH, WIDTH, edge, box_vert_pad_opt2<T,TS>, myTypeName + " box vert 2D padded opt2");
    test_conv( data_flatSrc, data_flatDst, HEIGHT-edge, WIDTH, WIDTH, edge, box_vert_pad_opt3<T,TS>, myTypeName + " box vert 2D padded opt3");
    test_conv( data_flatSrc, data_flatDst, HEIGHT-edge, WIDTH, WIDTH, edge, box_vert_pad_opt4<T,TS>, myTypeName + " box vert 2D padded opt4");
    test_conv( data_flatSrc, data_flatDst, HEIGHT-edge, WIDTH, WIDTH, edge, box_vert_pad_opt5<T,TS>, myTypeName + " box vert 2D padded opt5");
    test_conv( data_flatSrc, data_flatDst, HEIGHT-edge, WIDTH, WIDTH, edge, box_vert_pad_opt6<T,TS>, myTypeName + " box vert 2D padded opt6");
    
    std::string temp6( myTypeName + " convolution_box Vertical 2D padded" );
    summarize( temp6.c_str(), (HEIGHT-edge)*WIDTH, iterations, kDontShowGMeans, kDontShowPenalty );
    
    

    // 2D managing edge conditions - is kind of insane

    test_conv( data_flatSrc, data_flatDst, HEIGHT-edge, WIDTH-edge, WIDTH, edge, box_2D_pad<T,TS>, myTypeName + " box 2D padded");
    test_conv( data_flatSrc, data_flatDst, HEIGHT-edge, WIDTH-edge, WIDTH, edge, box_2D_pad_opt1<T,TS>, myTypeName + " box 2D padded opt1");
    test_conv( data_flatSrc, data_flatDst, HEIGHT-edge, WIDTH-edge, WIDTH, edge, box_2D_pad_opt2<T,TS>, myTypeName + " box 2D padded opt2");
    test_conv( data_flatSrc, data_flatDst, HEIGHT-edge, WIDTH-edge, WIDTH, edge, box_2D_pad_opt3<T,TS>, myTypeName + " box 2D padded opt3");
    
    std::string temp9( myTypeName + " convolution_box 2D" );
    summarize( temp9.c_str(), (HEIGHT-edge)*(WIDTH-edge), iterations, kDontShowGMeans, kDontShowPenalty );


    
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
    TestOneType<int8_t, int16_t>();
    
    TestOneType<uint16_t, uint32_t>();
    TestOneType<int16_t, int32_t>();

    iterations /= 2;
    TestOneType<uint32_t, uint64_t>();
    TestOneType<int32_t, int64_t>();
    
    TestOneType<uint64_t, uint64_t>();
    TestOneType<int64_t, int64_t>();


    // here float and double are about the same speed as small ints
    iterations *= 2;
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
