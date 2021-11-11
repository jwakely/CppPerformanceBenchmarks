/*
    Coppyright 2021 Chris Cox
    Distributed under the MIT License (see accompanying file LICENSE_1_0_0.txt
    or a copy at http://stlab.adobe.com/licenses.html )


Goal:  Test compiler optimizations related to matrix flips and rotates.
                (╯º□º）╯︵ ┻━┻           ┬─┬ノ(º_ºノ)


Assumptions:

    1) The compiler will recognize and optimize matrix rotation patterns
        for all simple data types.

    2) The compiler will apply appropriate loop transforms for less optimal
        matrix rotation patterns.





TODO:  more cache blocking

DEFERRED: 90 degree rotate inplace on non-square matrices may be possible with an 8 element ring.
        But the logic to keep it straight seems really ugly...

*/

#include "benchmark_stdint.hpp"
#include <stddef.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <math.h>
#include <deque>
#include <string>
#include <memory>
#include <iostream>
#include <algorithm>
#include <cassert>
#include "benchmark_results.h"
#include "benchmark_timer.h"
#include "benchmark_typenames.h"
#include "benchmark_algorithms.h"

/******************************************************************************/

// this constant may need to be adjusted to give reasonable minimum times
// For best results, times should be about 1.0 seconds for the minimum test run
int iterations = 120000;


// 300K items - intended to exceed the L1 cache
// odd values to exercise special cases in some functions
const int WIDTH = 501;
const int HEIGHT = 601;

const int SIZE = WIDTH*HEIGHT;


// initial value for filling our arrays, may be changed from the command line
double init_value = 3.0;

/******************************************************************************/

#include "benchmark_shared_tests.h"

/******************************************************************************/
/******************************************************************************/

// debugging utility
template <typename T>
void print_matrix(T *zz, int rows, int cols, int rowStep) {
    for (int j = 0 ; j < rows ; ++j ) {
        for (int k = 0; k < cols; ++k ) {
            std::cout << std::hex << zz[j*rowStep+k] << ", ";
        }
        std::cout << "\n";
    }
    std::cout << "\n";
}

/******************************************************************************/

template <>     // specialization because bloody std::cout treats int8 as ASCII instead of showing hex as requested
void print_matrix(int8_t *zz, int rows, int cols, int rowStep) {
    for (int j = 0 ; j < rows ; ++j ) {
        for (int k = 0; k < cols; ++k ) {
            std::cout << std::hex << (uint16_t)(zz[j*rowStep+k]) << ", ";
        }
        std::cout << "\n";
    }
    std::cout << "\n";
}

/******************************************************************************/

template <>     // specialization because bloody std::cout treats uint8 as ASCII instead of showing hex as requested
void print_matrix(uint8_t *zz, int rows, int cols, int rowStep) {
    for (int j = 0 ; j < rows ; ++j ) {
        for (int k = 0; k < cols; ++k ) {
            std::cout << std::hex << (uint16_t)(zz[j*rowStep+k]) << ", ";
        }
        std::cout << "\n";
    }
    std::cout << "\n";
}

/******************************************************************************/
/******************************************************************************/

template <typename T >
struct flipHorizontal1 {
    void operator()(const T *src, T *dst, size_t rows, size_t cols, int sRowStep, int dRowStep) {
        for ( size_t k = 0; k < rows; ++k ) {
            for ( size_t j = 0 ; j < cols ; ++j ) {
                dst[ k*dRowStep+j ] = src[ k*sRowStep+(cols-1-j) ];
            }
        }
    }
};

/******************************************************************************/

template <typename T >
struct flipHorizontal2 {
    void operator()(const T *src, T *dst, size_t rows, size_t cols, int sRowStep, int dRowStep) {
        for ( size_t k = 0; k < rows; ++k ) {
            for ( size_t j = 0 ; j < cols ; ++j ) {
                dst[ j ] = src[ (cols-1-j) ];
            }
            src += sRowStep;
            dst += dRowStep;
        }
    }
};

/******************************************************************************/

template <typename T >
struct flipHorizontal3 {
    void operator()(const T *src, T *dst, size_t rows, size_t cols, int sRowStep, int dRowStep) {
        for ( size_t k = 0; k < rows; ++k ) {
            auto srcTemp = src + cols-1;
            for ( size_t j = 0 ; j < cols ; ++j ) {
                dst[ j ] = *srcTemp--;
            }
            src += sRowStep;
            dst += dRowStep;
        }
    }
};

/******************************************************************************/

// use STL reverse_copy instead of our own loop
template <typename T >
struct flipHorizontal4 {
    void operator()(const T *src, T *dst, size_t rows, size_t cols, int sRowStep, int dRowStep) {
        for ( size_t k = 0; k < rows; ++k ) {
            std::reverse_copy( src, src+cols, dst );
            src += sRowStep;
            dst += dRowStep;
        }
    }
};

/******************************************************************************/
/******************************************************************************/

template <typename T >
struct flipHorizontal_inplace1 {
    void operator()(T *src, size_t rows, size_t cols, int sRowStep) {
        for ( size_t k = 0; k < rows; ++k ) {
            for ( size_t j = 0 ; j < (cols/2) ; ++j ) {
                std::swap( src[ k*sRowStep + j ], src[ k*sRowStep+(cols-1-j) ] );
            }
        }
    }
};

/******************************************************************************/

template <typename T >
struct flipHorizontal_inplace2 {
    void operator()(T *src, size_t rows, size_t cols, int sRowStep) {
        for ( size_t k = 0; k < rows; ++k ) {
            std::reverse( src + k*sRowStep, src + k*sRowStep+cols );
        }
    }
};

/******************************************************************************/

template <typename T >
struct flipHorizontal_inplace3 {
    void operator()(T *src, size_t rows, size_t cols, int sRowStep) {
        for ( size_t k = 0; k < rows; ++k ) {
            std::reverse( src, src+cols );
            src += sRowStep;
        }
    }
};

/******************************************************************************/

template <typename T >
struct flipHorizontal_inplace4 {
    void operator()(T *src, size_t rows, size_t cols, int sRowStep) {
        for ( size_t k = 0; k < rows; ++k ) {
            for ( size_t j = 0 ; j < (cols/2) ; ++j ) {
                std::swap( src[ j ], src[ cols-1-j ] );
            }
            src += sRowStep;
        }
    }
};

/******************************************************************************/

// use incremented pointers
template <typename T >
struct flipHorizontal_inplace5 {
    void operator()(T *src, size_t rows, size_t cols, int sRowStep) {
        for ( size_t k = 0; k < rows; ++k ) {
            auto srcF = src;
            auto srcB = src + cols-1;
            for ( size_t j = 0 ; j < (cols/2) ; ++j ) {
                std::swap( *srcF++, *srcB-- );
            }
            src += sRowStep;
        }
    }
};

/******************************************************************************/

// use an explicit swap instead of std::swap
template <typename T >
struct flipHorizontal_inplace6 {
    void operator()(T *src, size_t rows, size_t cols, int sRowStep) {
        for ( size_t k = 0; k < rows; ++k ) {
            for ( size_t j = 0 ; j < (cols/2) ; ++j ) {
                auto tmp = src[ j ];
                src[ j ] = src[ cols-1-j ];
                src[ cols-1-j ] = tmp;
            }
            src += sRowStep;
        }
    }
};

/******************************************************************************/

// use temp buffer instead of swap
template <typename T >
struct flipHorizontal_inplace7 {
    void operator()(T *src, size_t rows, size_t cols, int sRowStep) {
        const size_t blockSize = 1600/sizeof(T);
        T tempBuffer[blockSize];
        for ( size_t k = 0; k < rows; ++k ) {
            for ( size_t jj = 0; jj < (cols/2); jj += blockSize ) {
                int jend = jj + blockSize;
                if (jend > (cols/2))    jend = (cols/2);
                auto tempFake = tempBuffer - jj;

                // copy end backwards into buffer
                for ( size_t j = jj; j < jend; ++j )
                    tempFake[ j ] = src[ cols-1-j ];

                // copy start backwards into end
                for ( size_t j = jj; j < jend; ++j)
                    src[ cols-1-j ] = src[ j ];

                // copy buffer into start (aka memcpy)
                for ( size_t j = jj; j < jend; ++j)
                    src[ j ] = tempFake[ j ];
            }
            src += sRowStep;
        }
    }
};

/******************************************************************************/

// use temp buffer instead of swap
// use standard templates instead of our own loops
template <typename T >
struct flipHorizontal_inplace8 {
    void operator()(T *src, size_t rows, size_t cols, int sRowStep) {
        const size_t blockSize = 1600/sizeof(T);
        T tempBuffer[blockSize];
        for ( size_t k = 0; k < rows; ++k ) {
            for ( size_t jj = 0; jj < (cols/2); jj += blockSize ) {
                int jend = jj + blockSize;
                if (jend > (cols/2))    jend = (cols/2);
                auto srcStart = src + jj;
                auto srcEnd = src+cols-1-jend+1;
                auto srcEnd2 = src+cols-1-jj+1;

                // copy rightside backwards into buffer
                std::reverse_copy( srcEnd, srcEnd2, tempBuffer );

                // copy leftside backwards into rightside
                std::reverse_copy( srcStart, src+jend, srcEnd );

                // copy buffer into leftside (aka memcpy)
                std::copy_n( tempBuffer, (jend-jj), srcStart );
            }
            src += sRowStep;
        }
    }
};

/******************************************************************************/
/******************************************************************************/

template <typename T >
struct flipVertical1 {
    void operator()(const T *src, T *dst, size_t rows, size_t cols, int sRowStep, int dRowStep) {
        for ( size_t k = 0; k < rows; ++k ) {
            for ( size_t j = 0 ; j < cols ; ++j ) {
                dst[ k*dRowStep+j ] = src[ (rows-1-k)*sRowStep+j ];
            }
        }
    }
};

/******************************************************************************/

template <typename T >
struct flipVertical2 {
    void operator()(const T *src, T *dst, size_t rows, size_t cols, int sRowStep, int dRowStep) {
        src += (rows-1)*sRowStep;
        for ( size_t k = 0; k < rows; ++k ) {
            for ( size_t j = 0 ; j < cols ; ++j ) {
                dst[ j ] = src[ j ];
            }
            src -= sRowStep;
            dst += dRowStep;
        }
    }
};

/******************************************************************************/

template <typename T >
struct flipVertical3 {
    void operator()(const T *src, T *dst, size_t rows, size_t cols, int sRowStep, int dRowStep) {
        src += (rows-1)*sRowStep;
        for ( size_t k = 0; k < rows; ++k ) {
            memcpy( dst, src, cols*sizeof(T) );
            src -= sRowStep;
            dst += dRowStep;
        }
    }
};

/******************************************************************************/

template <typename T >
struct flipVertical4 {
    void operator()(const T *src, T *dst, size_t rows, size_t cols, int sRowStep, int dRowStep) {
        src += (rows-1)*sRowStep;
        for ( size_t k = 0; k < rows; ++k ) {
            std::copy_n( src, cols, dst );
            src -= sRowStep;
            dst += dRowStep;
        }
    }
};

/******************************************************************************/

template <typename T >
struct flipVertical5 {
    void operator()(const T *src, T *dst, size_t rows, size_t cols, int sRowStep, int dRowStep) {
        src += (rows-1)*sRowStep;
        for ( size_t k = 0; k < rows; ++k ) {
            std::copy( src, src+cols, dst );
            src -= sRowStep;
            dst += dRowStep;
        }
    }
};

/******************************************************************************/
/******************************************************************************/

template <typename T >
struct flipVertical_inplace1 {
    void operator()(T *src, size_t rows, size_t cols, int sRowStep) {
        for ( size_t k = 0; k < (rows/2); ++k ) {
            for ( size_t j = 0 ; j < cols ; ++j ) {
                std::swap( src[ k*sRowStep + j ], src[ (rows-1-k)*sRowStep + j ] );
            }
        }
    }
};

/******************************************************************************/

template <typename T >
struct flipVertical_inplace2 {
    void operator()(T *src, size_t rows, size_t cols, int sRowStep) {
        auto srcTop = src;
        auto srcBottom = src + (rows-1)*sRowStep;
        for ( size_t k = 0; k < (rows/2); ++k ) {
            for ( size_t j = 0 ; j < cols ; ++j ) {
                std::swap( srcTop[ j ], srcBottom[ j ] );
            }
            srcTop += sRowStep;
            srcBottom -= sRowStep;
        }
    }
};

/******************************************************************************/

template <typename T >
struct flipVertical_inplace3 {
    void operator()(T *src, size_t rows, size_t cols, int sRowStep) {
        auto srcTop = src;
        auto srcBottom = src + (rows-1)*sRowStep;
        for ( size_t k = 0; k < (rows/2); ++k ) {
            std::swap_ranges( srcTop, srcTop+cols, srcBottom );
            srcTop += sRowStep;
            srcBottom -= sRowStep;
        }
    }
};

/******************************************************************************/

// use incremented pointers
template <typename T >
struct flipVertical_inplace4 {
    void operator()(T *src, size_t rows, size_t cols, int sRowStep) {
        auto srcTop = src;
        auto srcBottom = src + (rows-1)*sRowStep;
        for ( size_t k = 0; k < (rows/2); ++k ) {
            for ( size_t j = 0 ; j < cols ; ++j ) {
                std::swap( *srcTop++, *srcBottom++ );
            }
            srcTop += sRowStep-cols;
            srcBottom -= sRowStep+cols;
        }
    }
};

/******************************************************************************/

// use an explicit swap
template <typename T >
struct flipVertical_inplace5 {
    void operator()(T *src, size_t rows, size_t cols, int sRowStep) {
        auto srcTop = src;
        auto srcBottom = src + (rows-1)*sRowStep;
        for ( size_t k = 0; k < (rows/2); ++k ) {
            for ( size_t j = 0 ; j < cols ; ++j ) {
                auto tmp = srcTop[ j ];
                srcTop[ j ] = srcBottom[ j ];
                srcBottom[ j ] = tmp;
            }
            srcTop += sRowStep;
            srcBottom -= sRowStep;
        }
    }
};

/******************************************************************************/

// use a temp buffer instead of swap
template <typename T >
struct flipVertical_inplace6 {
    void operator()(T *src, size_t rows, size_t cols, int sRowStep) {
        const size_t blockSize = 1600/sizeof(T);
        T tempBuffer[blockSize];
        auto srcTop = src;
        auto srcBottom = src + (rows-1)*sRowStep;
        
        for ( size_t k = 0; k < (rows/2); ++k ) {
            for ( size_t jj = 0; jj < cols; jj += blockSize ) {
                int jend = jj + blockSize;
                if (jend > cols)    jend = cols;
                
                // copy top into temp
                for ( size_t j = jj ; j < jend ; ++j )
                    tempBuffer[ j-jj ] = srcTop[ j ];
                
                // copy bottom into top
                for ( size_t j = jj ; j < jend ; ++j )
                    srcTop[ j ] = srcBottom[ j ];
                
                // copy temp into bottom
                for ( size_t j = jj ; j < jend ; ++j )
                    srcBottom[j] = tempBuffer[ j-jj ];
            }
            srcTop += sRowStep;
            srcBottom -= sRowStep;
        }
    }
};

/******************************************************************************/

// use a temp buffer instead of swap
// use std templates instead of our own loops
template <typename T >
struct flipVertical_inplace7 {
    void operator()(T *src, size_t rows, size_t cols, int sRowStep) {
        const size_t blockSize = 1600/sizeof(T);
        T tempBuffer[blockSize];
        auto srcTop = src;
        auto srcBottom = src + (rows-1)*sRowStep;
        
        for ( size_t k = 0; k < (rows/2); ++k ) {
            for ( size_t jj = 0; jj < cols; jj += blockSize ) {
                int jend = jj + blockSize;
                if (jend > cols)    jend = cols;
                
                // copy top into temp
                std::copy_n( srcTop+jj, jend-jj, tempBuffer );
                
                // copy bottom into top
                std::copy_n( srcBottom+jj, jend-jj, srcTop+jj );
                
                // copy temp into bottom
                std::copy_n( tempBuffer, jend-jj, srcBottom+jj );
            }
            srcTop += sRowStep;
            srcBottom -= sRowStep;
        }
    }
};

/******************************************************************************/

// use a temp buffer instead of swap
// use memcpy instead of our own loops
template <typename T >
struct flipVertical_inplace8 {
    void operator()(T *src, size_t rows, size_t cols, int sRowStep) {
        const size_t blockSize = 1600/sizeof(T);
        T tempBuffer[blockSize];
        auto srcTop = src;
        auto srcBottom = src + (rows-1)*sRowStep;
        
        for ( size_t k = 0; k < (rows/2); ++k ) {
            for ( size_t jj = 0; jj < cols; jj += blockSize ) {
                int jend = jj + blockSize;
                if (jend > cols)    jend = cols;
                
                // copy top into temp
                memcpy( tempBuffer, srcTop+jj, (jend-jj)*sizeof(T) );
                
                // copy bottom into top
                memcpy( srcTop+jj, srcBottom+jj, (jend-jj)*sizeof(T) );
                
                // copy temp into bottom
                memcpy( srcBottom+jj, tempBuffer, (jend-jj)*sizeof(T) );
            }
            srcTop += sRowStep;
            srcBottom -= sRowStep;
        }
    }
};

/******************************************************************************/
/******************************************************************************/

template <typename T >
struct flip180_1 {
    void operator()(const T *src, T *dst, size_t rows, size_t cols, int sRowStep, int dRowStep) {
        for ( size_t k = 0; k < rows; ++k ) {
            for ( size_t j = 0 ; j < cols ; ++j ) {
                dst[ k*dRowStep+j ] = src[ (rows-1-k)*sRowStep+(cols-1-j) ];
            }
        }
    }
};

/******************************************************************************/

template <typename T >
struct flip180_2 {
    void operator()(const T *src, T *dst, size_t rows, size_t cols, int sRowStep, int dRowStep) {
        src += (rows-1)*sRowStep;
        for ( size_t k = 0; k < rows; ++k ) {
            for ( size_t j = 0 ; j < cols ; ++j ) {
                dst[ j ] = src[ (cols-1-j) ];
            }
            src -= sRowStep;
            dst += dRowStep;
        }
    }
};

/******************************************************************************/

template <typename T >
struct flip180_3 {
    void operator()(const T *src, T *dst, size_t rows, size_t cols, int sRowStep, int dRowStep) {
        src += (rows-1)*sRowStep + cols - 1;
        for ( size_t k = 0; k < rows; ++k ) {
            auto srcTemp = src;
            for ( size_t j = 0 ; j < cols ; ++j ) {
                dst[ j ] = *srcTemp--;
            }
            src -= sRowStep;
            dst += dRowStep;
        }
    }
};

/******************************************************************************/

template <typename T >
struct flip180_4 {
    void operator()(const T *src, T *dst, size_t rows, size_t cols, int sRowStep, int dRowStep) {
        flipVertical1<T>()( src, dst, rows, cols, sRowStep, dRowStep );
        flipHorizontal_inplace1<T>()( dst, rows, cols, dRowStep );
    }
};

/******************************************************************************/

template <typename T >
struct flip180_5 {
    void operator()(const T *src, T *dst, size_t rows, size_t cols, int sRowStep, int dRowStep) {
        flipHorizontal1<T>()( src, dst, rows, cols, sRowStep, dRowStep );
        flipVertical_inplace1<T>()( dst, rows, cols, dRowStep );
    }
};

/******************************************************************************/
/******************************************************************************/

template <typename T >
struct flip180_inplace1 {
    void operator()( T *src, size_t rows, size_t cols, int sRowStep) {
        for ( size_t k = 0; k < (rows/2); ++k ) {
            for ( size_t j = 0 ; j < cols; ++j ) {
                std::swap( src[ k*sRowStep+j ], src[ (rows-1-k)*sRowStep+(cols-1-j) ] );
            }
        }
        if ((rows & 1) != 0) {      // single middle row still has to be flipped
            size_t k = (rows/2);
            for ( size_t j = 0 ; j < (cols/2); ++j ) {
                std::swap( src[ k*sRowStep+j ], src[ k*sRowStep+(cols-1-j) ] );
            }
        }
    }
};

/******************************************************************************/

template <typename T >
struct flip180_inplace2 {
    void operator()( T *src, size_t rows, size_t cols, int sRowStep) {
        auto srcTop = src;
        auto srcBottom = src + (rows-1)*sRowStep;
        for ( size_t k = 0; k < (rows/2); ++k ) {
            for ( size_t j = 0 ; j < cols; ++j ) {
                std::swap( srcTop[ j ], srcBottom[ (cols-1-j) ] );
            }
            srcTop += sRowStep;
            srcBottom -= sRowStep;
        }
        if ((rows & 1) != 0) {      // single middle row still has to be flipped
            //assert( srcTop == src + (rows/2)*sRowStep );
            //srcTop = src + (rows/2)*sRowStep;
            for ( size_t j = 0 ; j < (cols/2); ++j ) {
                std::swap( srcTop[ j ], srcTop[ (cols-1-j) ] );
            }
        }
        
    }
};

/******************************************************************************/

template <typename T >
struct flip180_inplace3 {
    void operator()( T *src, size_t rows, size_t cols, int sRowStep) {
        flipVertical_inplace1<T>()( src, rows, cols, sRowStep );
        flipHorizontal_inplace1<T>()( src, rows, cols, sRowStep );
    }
};

/******************************************************************************/

// use temp buffer instead of swap
template <typename T >
struct flip180_inplace4 {
    void operator()( T *src, size_t rows, size_t cols, int sRowStep) {
        const size_t blockSize = 1600/sizeof(T);
        T tempBuffer[blockSize];
        auto srcTop = src;
        auto srcBottom = src + (rows-1)*sRowStep;
        for ( size_t k = 0; k < (rows/2); ++k ) {
            for ( size_t jj = 0; jj < cols; jj += blockSize ) {
                int jend = jj + blockSize;
                if (jend > cols)    jend = cols;
                auto tempFake = tempBuffer - jj;
                
                // copy bottom backwards into temp
                for ( size_t j = jj ; j < jend; ++j )
                    tempFake[ j ] = srcBottom[ (cols-1-j) ];
                
                // copy top backwards into bottom
                for ( size_t j = jj ; j < jend; ++j )
                    srcBottom[ (cols-1-j) ] = srcTop[ j ];
                
                // copy temp into top (aka memcpy)
                for ( size_t j = jj ; j < jend; ++j )
                    srcTop[ j ] = tempFake[ j ];
            }
            srcTop += sRowStep;
            srcBottom -= sRowStep;
        }
        if ((rows & 1) != 0) {      // single middle row still has to be flipped
            //assert( srcTop == src + (rows/2)*sRowStep );
            //srcTop = src + (rows/2)*sRowStep;
            for ( size_t jj = 0; jj < (cols/2); jj += blockSize ) {
                int jend = jj + blockSize;
                if (jend > (cols/2))    jend = (cols/2);
                auto tempFake = tempBuffer - jj;

                // copy end backwards into buffer
                for ( size_t j = jj; j < jend; ++j )
                    tempFake[ j ] = srcTop[ cols-1-j ];

                // copy start backwards into end
                for ( size_t j = jj; j < jend; ++j)
                    srcTop[ cols-1-j ] = srcTop[ j ];

                // copy buffer into start (aka memcpy)
                for ( size_t j = jj; j < jend; ++j)
                    srcTop[ j ] = tempFake[ j ];
            }
        }
        
    }
};

/******************************************************************************/

// use temp buffer instead of swap
// use standard templates instead of our own loops
template <typename T >
struct flip180_inplace5 {
    void operator()( T *src, size_t rows, size_t cols, int sRowStep) {
        const size_t blockSize = 1600/sizeof(T);
        T tempBuffer[blockSize];
        auto srcTop = src;
        auto srcBottom = src + (rows-1)*sRowStep;
        for ( size_t k = 0; k < (rows/2); ++k ) {
            for ( size_t jj = 0; jj < cols; jj += blockSize ) {
                int jend = jj + blockSize;
                if (jend > cols)    jend = cols;
                auto topStart = srcTop + jj;
                auto botEnd = srcBottom+cols-1-jend+1;
                auto botEnd2 = srcBottom+cols-1-jj+1;
                
                // copy bottom backwards into temp
                std::reverse_copy( botEnd, botEnd2, tempBuffer );

                // copy top backwards into bottom
                std::reverse_copy( topStart, srcTop+jend, botEnd );
                
                // copy temp into top (aka memcpy)
                std::copy_n( tempBuffer, (jend-jj), topStart );
            }
            srcTop += sRowStep;
            srcBottom -= sRowStep;
        }
        if ((rows & 1) != 0) {      // single middle row still has to be flipped
            //assert( srcTop == src + (rows/2)*sRowStep );
            //srcTop = src + (rows/2)*sRowStep;
            for ( size_t jj = 0; jj < (cols/2); jj += blockSize ) {
                int jend = jj + blockSize;
                if (jend > (cols/2))    jend = (cols/2);
                auto srcStart = srcTop + jj;
                auto srcEnd = srcTop+cols-1-jend+1;
                auto srcEnd2 = srcTop+cols-1-jj+1;

                // copy rightside backwards into buffer
                std::reverse_copy( srcEnd, srcEnd2, tempBuffer );

                // copy leftside backwards into rightside
                std::reverse_copy( srcStart, srcTop+jend, srcEnd );

                // copy buffer into leftside (aka memcpy)
                std::copy_n( tempBuffer, (jend-jj), srcStart );
            }
        }
        
    }
};

/******************************************************************************/
/******************************************************************************/

template <typename T >
struct transpose1 {
    void operator()(const T *src, T *dst, size_t rows, size_t cols, int sRowStep, int dRowStep) {
        for ( size_t k = 0; k < cols; ++k ) {
            for ( size_t j = 0 ; j < rows ; ++j ) {
                dst[ k*dRowStep+j ] = src[ j*sRowStep+k ];
            }
        }
    }
};

/******************************************************************************/

// invert loops
template <typename T >
struct transpose2 {
    void operator()(const T *src, T *dst, size_t rows, size_t cols, int sRowStep, int dRowStep) {
        for ( size_t j = 0 ; j < rows ; ++j ) {
            for ( size_t k = 0; k < cols; ++k ) {
                dst[ k*dRowStep+j ] = src[ j*sRowStep+k ];
            }
        }
    }
};

/******************************************************************************/

template <typename T >
struct transpose3 {
    void operator()(const T *src, T *dst, size_t rows, size_t cols, int sRowStep, int dRowStep) {
        for ( size_t k = 0; k < cols; ++k ) {
            for ( size_t j = 0 ; j < rows ; ++j ) {
                dst[ j ] = src[ j*sRowStep ];
            }
            src += 1;
            dst += dRowStep;
        }
    }
};

/******************************************************************************/

// unroll one loop for slightly better cache usage
template <typename T >
struct transpose4 {
    void operator()(const T *src, T *dst, size_t rows, size_t cols, int sRowStep, int dRowStep) {
        size_t k;
        for ( k = 0; k < (cols-3); k += 4 ) {
            for ( size_t j = 0 ; j < rows ; ++j ) {
                dst[ dRowStep*0 + j ] = src[ j*sRowStep + 0 ];
                dst[ dRowStep*1 + j ] = src[ j*sRowStep + 1 ];
                dst[ dRowStep*2 + j ] = src[ j*sRowStep + 2 ];
                dst[ dRowStep*3 + j ] = src[ j*sRowStep + 3 ];
            }
            src += 4;
            dst += 4*dRowStep;
        }
        for ( ; k < cols; ++k ) {
            for ( size_t j = 0 ; j < rows ; ++j ) {
                dst[ j ] = src[ j*sRowStep ];
            }
            src += 1;
            dst += dRowStep;
        }
    }
};

/******************************************************************************/

// unroll 2 loops for slightly better cache usage
template <typename T >
struct transpose5 {
    void operator()(const T *src, T *dst, size_t rows, size_t cols, int sRowStep, int dRowStep) {
        size_t j, k;
        for ( k = 0; k < (cols-3); k += 4 ) {
            for ( j = 0 ; j < (rows-3) ; j += 4 ) {
                dst[ dRowStep*0 + (j+0) ] = src[ (j+0)*sRowStep + 0 ];
                dst[ dRowStep*0 + (j+1) ] = src[ (j+1)*sRowStep + 0 ];
                dst[ dRowStep*0 + (j+2) ] = src[ (j+2)*sRowStep + 0 ];
                dst[ dRowStep*0 + (j+3) ] = src[ (j+3)*sRowStep + 0 ];
                
                dst[ dRowStep*1 + (j+0) ] = src[ (j+0)*sRowStep + 1 ];
                dst[ dRowStep*1 + (j+1) ] = src[ (j+1)*sRowStep + 1 ];
                dst[ dRowStep*1 + (j+2) ] = src[ (j+2)*sRowStep + 1 ];
                dst[ dRowStep*1 + (j+3) ] = src[ (j+3)*sRowStep + 1 ];
                
                dst[ dRowStep*2 + (j+0) ] = src[ (j+0)*sRowStep + 2 ];
                dst[ dRowStep*2 + (j+1) ] = src[ (j+1)*sRowStep + 2 ];
                dst[ dRowStep*2 + (j+2) ] = src[ (j+2)*sRowStep + 2 ];
                dst[ dRowStep*2 + (j+3) ] = src[ (j+3)*sRowStep + 2 ];
                
                dst[ dRowStep*3 + (j+0) ] = src[ (j+0)*sRowStep + 3 ];
                dst[ dRowStep*3 + (j+1) ] = src[ (j+1)*sRowStep + 3 ];
                dst[ dRowStep*3 + (j+2) ] = src[ (j+2)*sRowStep + 3 ];
                dst[ dRowStep*3 + (j+3) ] = src[ (j+3)*sRowStep + 3 ];
            }
            for ( ; j < rows ; ++j ) {
                dst[ dRowStep*0 + j ] = src[ j*sRowStep + 0 ];
                dst[ dRowStep*1 + j ] = src[ j*sRowStep + 1 ];
                dst[ dRowStep*2 + j ] = src[ j*sRowStep + 2 ];
                dst[ dRowStep*3 + j ] = src[ j*sRowStep + 3 ];
            }
            src += 4;
            dst += 4*dRowStep;
        }
        for ( ; k < cols; ++k ) {
            for ( j = 0 ; j < rows ; ++j ) {
                dst[ j ] = src[ j*sRowStep ];
            }
            src += 1;
            dst += dRowStep;
        }
    }
};

/******************************************************************************/
/*
The block size makes very little difference in performance
 */
 
// cache block in cols only to improve cache coherence
template <typename T >
struct transpose6 {
    void operator()(const T *src, T *dst, size_t rows, size_t cols, int sRowStep, int dRowStep) {
        size_t kk;
        
        const int blockSize = 200/sizeof(T);
        for ( kk = 0; kk < cols; kk += blockSize ) {
            int kend = kk + blockSize;
            if (kend > cols)    kend = cols;
                
                auto src2 = src;
                auto dst2 = dst;
                for ( size_t k = kk; k < kend; ++k ) {
                    for ( size_t j = 0 ; j < rows ; ++j ) {
                        dst2[ j ] = src2[ j*sRowStep ];
                    }
                    src2 += 1;
                    dst2 += dRowStep;
                }
            src += blockSize;
            dst += blockSize * dRowStep;
        }
    }
};

/******************************************************************************/

// cache block in rows only to improve cache coherence
template <typename T >
struct transpose7 {
    void operator()(const T *src, T *dst, size_t rows, size_t cols, int sRowStep, int dRowStep) {
        const size_t blockSize = 220/sizeof(T);
        size_t jj;

        for ( jj = 0; jj < rows; jj += blockSize ) {
            int jend = jj + blockSize;
            if (jend > cols)    jend = rows;
            
            auto src2 = src;
            auto dst2 = dst;
            for ( size_t k = 0; k < cols; ++k ) {
                for ( size_t j = jj ; j < jend ; ++j ) {
                    dst2[ j ] = src2[ j*sRowStep ];
                }
                src2 += 1;
                dst2 += dRowStep;
            }
        }

    }
};


/******************************************************************************/
 
// cache block in rows and cols to improve cache coherence
template <typename T >
struct transpose8 {
    void operator()(const T *src, T *dst, size_t rows, size_t cols, int sRowStep, int dRowStep) {
        const size_t blockSize = 333/sizeof(T);
        size_t kk, jj;
        for ( kk = 0; kk < cols; kk += blockSize ) {
            int kend = kk + blockSize;
            if (kend > cols)    kend = cols;

            for ( jj = 0; jj < rows; jj += blockSize ) {
                int jend = jj + blockSize;
                if (jend > cols)    jend = rows;
                
                auto src2 = src;
                auto dst2 = dst;
                for ( size_t k = kk; k < kend; ++k ) {
                    for ( size_t j = jj ; j < jend ; ++j ) {
                        dst2[ j ] = src2[ j*sRowStep ];
                    }
                    src2 += 1;
                    dst2 += dRowStep;
                }
            }
            src += blockSize;
            dst += blockSize * dRowStep;
        }
    }
};

/******************************************************************************/

// cache block to improve cache coherence, plus loop unrolling
template <typename T >
struct transpose9 {
    void operator()(const T *src, T *dst, size_t rows, size_t cols, int sRowStep, int dRowStep) {
        const size_t blockSize = 208/sizeof(T);
        size_t kk, jj;
        
        for ( kk = 0; kk < cols; kk += blockSize ) {
            int kend = kk + blockSize;
            if (kend > cols)    kend = cols;

            for ( jj = 0; jj < rows; jj += blockSize ) {
                int jend = jj + blockSize;
                if (jend > cols)    jend = rows;
                
                auto src2 = src;
                auto dst2 = dst;
                size_t j, k;
                for ( k = kk; k < (kend-3); k += 4 ) {
                    for ( j = jj ; j < (jend-3) ; j += 4 ) {
                        dst2[ dRowStep*0 + (j+0) ] = src2[ (j+0)*sRowStep + 0 ];
                        dst2[ dRowStep*0 + (j+1) ] = src2[ (j+1)*sRowStep + 0 ];
                        dst2[ dRowStep*0 + (j+2) ] = src2[ (j+2)*sRowStep + 0 ];
                        dst2[ dRowStep*0 + (j+3) ] = src2[ (j+3)*sRowStep + 0 ];
                        
                        dst2[ dRowStep*1 + (j+0) ] = src2[ (j+0)*sRowStep + 1 ];
                        dst2[ dRowStep*1 + (j+1) ] = src2[ (j+1)*sRowStep + 1 ];
                        dst2[ dRowStep*1 + (j+2) ] = src2[ (j+2)*sRowStep + 1 ];
                        dst2[ dRowStep*1 + (j+3) ] = src2[ (j+3)*sRowStep + 1 ];
                        
                        dst2[ dRowStep*2 + (j+0) ] = src2[ (j+0)*sRowStep + 2 ];
                        dst2[ dRowStep*2 + (j+1) ] = src2[ (j+1)*sRowStep + 2 ];
                        dst2[ dRowStep*2 + (j+2) ] = src2[ (j+2)*sRowStep + 2 ];
                        dst2[ dRowStep*2 + (j+3) ] = src2[ (j+3)*sRowStep + 2 ];
                        
                        dst2[ dRowStep*3 + (j+0) ] = src2[ (j+0)*sRowStep + 3 ];
                        dst2[ dRowStep*3 + (j+1) ] = src2[ (j+1)*sRowStep + 3 ];
                        dst2[ dRowStep*3 + (j+2) ] = src2[ (j+2)*sRowStep + 3 ];
                        dst2[ dRowStep*3 + (j+3) ] = src2[ (j+3)*sRowStep + 3 ];
                    }
                    for ( ; j < jend ; ++j ) {
                        dst2[ dRowStep*0 + j ] = src2[ j*sRowStep + 0 ];
                        dst2[ dRowStep*1 + j ] = src2[ j*sRowStep + 1 ];
                        dst2[ dRowStep*2 + j ] = src2[ j*sRowStep + 2 ];
                        dst2[ dRowStep*3 + j ] = src2[ j*sRowStep + 3 ];
                    }
                    src2 += 4;
                    dst2 += 4*dRowStep;
                }
                for ( ; k < kend; ++k ) {
                    for ( j = jj ; j < jend ; ++j ) {
                        dst2[ j ] = src2[ j*sRowStep ];
                    }
                    src2 += 1;
                    dst2 += dRowStep;
                }
            }
            src += blockSize;
            dst += blockSize * dRowStep;
        }
    }
};

/******************************************************************************/
/******************************************************************************/

// assert( rows == cols );
template <typename T >
struct transpose_inplace1 {
    void operator()(T *src, size_t cols, int sRowStep) {
        for ( size_t k = 0; k < cols; ++k ) {
            for ( size_t j = k+1 ; j < cols ; ++j ) {     // because we're flipping across the diagonal
                std::swap( src[ k*sRowStep+j ], src[ j*sRowStep+k ] );
            }
        }
    }
};

/******************************************************************************/

// assert( rows == cols );
// invert loops
template <typename T >
struct transpose_inplace2 {
    void operator()(T *src, size_t cols, int sRowStep) {
        for ( size_t j = 0 ; j < cols ; ++j ) {
            for ( size_t k = j+1; k < cols; ++k ) {     // because we're flipping across the diagonal
                std::swap( src[ k*sRowStep+j ], src[ j*sRowStep+k ] );
            }
        }
    }
};

/******************************************************************************/

// assert( rows == cols );
template <typename T >
struct transpose_inplace3 {
    void operator()(T *src, size_t cols, int sRowStep) {
        auto src1 = src;
        auto src2 = src;
        for ( size_t k = 0; k < cols; ++k ) {
            for ( size_t j = k+1 ; j < cols ; ++j ) {     // because we're flipping across the diagonal
                std::swap( src1[ j ], src2[ j*sRowStep ] );
            }
            src1 += sRowStep;
            src2 += 1;
        }
    }
};

/******************************************************************************/

// assert( rows == cols );
// work in stripes
template <typename T >
struct transpose_inplace4 {
    void operator()(T *src, size_t cols, int sRowStep) {
        const size_t blockSize = 8;
        size_t kk;
        for ( kk = 0; kk < cols; kk += blockSize ) {
            int kend = kk + blockSize;
            if (kend > cols)    kend = cols;
            auto blockLimit = kend-kk;
            
            // align each row until we're past the diagonal for this block
            for (size_t m = 0; m < blockLimit; ++m) {
                for ( size_t j = kk+1+m ; j < kend; ++j ) {     // because we're flipping across the diagonal
                    std::swap( src[ (kk+m)*sRowStep+j ], src[ j*sRowStep+(kk+m) ] );
                }
            }
            
            // do full blocksize sweep
            for (size_t m = 0; m < blockLimit; ++m) {
                for (size_t j = kend; j < cols; ++j ) {
                    std::swap( src[ (kk+m)*sRowStep+j ], src[ j*sRowStep+(kk+m) ] );
                }
            }
            
        }
    }
};

/******************************************************************************/

// assert( rows == cols );
// work in stripes, expanded
template <typename T >
struct transpose_inplace5 {
    void operator()(T *src, size_t cols, int sRowStep) {
        const size_t blockSize = 16;
        size_t kk;
        for ( kk = 0; kk < (cols-blockSize-1); kk += blockSize ) {
            int kend = kk + blockSize;
            
            // align each row until we're past the diagonal for this block
            for (size_t m = 0; m < blockSize; ++m) {
                for ( size_t j = kk+1+m ; j < kend; ++j ) {     // because we're flipping across the diagonal
                    std::swap( src[ (kk+m)*sRowStep+j ], src[ j*sRowStep+(kk+m) ] );
                }
            }
            
            // do full blocksize sweep
            for (size_t j = kend; j < cols; ++j ) {
                for (size_t m = 0; m < blockSize; ++m) {
                    std::swap( src[ (kk+m)*sRowStep+j ], src[ j*sRowStep+(kk+m) ] );
                }
            }
        }
        for ( ; kk < cols; ++kk ) {
            for ( size_t j = kk+1 ; j < cols; ++j ) {     // because we're flipping across the diagonal
                std::swap( src[ (kk)*sRowStep+j ], src[ j*sRowStep+(kk) ] );
            }
        }
    }
};

/******************************************************************************/

// assert( rows == cols );
// work in stripes
// unroll a bit to work on blocks
template <typename T >
struct transpose_inplace6 {
    void operator()(T *src, size_t cols, int sRowStep) {
        const size_t blockSize = 16;
        size_t kk;
        for ( kk = 0; kk < (cols-blockSize-1); kk += blockSize ) {
            int kend = kk + blockSize;
            
            // align each row until we're past the diagonal for this block
            for (size_t m = 0; m < blockSize; ++m) {
                for ( size_t j = kk+1+m ; j < kend; ++j ) {     // because we're flipping across the diagonal
                    std::swap( src[ (kk+m)*sRowStep+j ], src[ j*sRowStep+(kk+m) ] );
                }
            }
            
            // do full blocksize sweep in chunks
            size_t j;
            for ( j = kend; j < (cols-blockSize-1); j += blockSize ) {
                for (size_t m = 0; m < blockSize; ++m) {
                    for (size_t n = 0; n < blockSize; ++n) {
                        std::swap( src[ (kk+m)*sRowStep+j+n ], src[ (j+n)*sRowStep+(kk+m) ] );
                    }
                }
            }
            for ( ; j < cols; ++j ) {
                for (size_t m = 0; m < blockSize; ++m) {
                    std::swap( src[ (kk+m)*sRowStep+j ], src[ j*sRowStep+(kk+m) ] );
                }
            }
        }
        for ( ; kk < cols; ++kk ) {
            for ( size_t j = kk+1 ; j < cols; ++j ) {     // because we're flipping across the diagonal
                std::swap( src[ (kk)*sRowStep+j ], src[ j*sRowStep+(kk) ] );
            }
        }
    }
};

/******************************************************************************/

// assert( rows == cols );
// work in blocks
// use a temporary block instead of swap
// NOTE - optimal block size is different for various data sizes
template <typename T >
struct transpose_inplace7 {
    void operator()(T *src, size_t cols, int sRowStep) {
        const size_t blockSize = 8;
        T tempBuffer[ blockSize*blockSize ];
        size_t kk;
        for ( kk = 0; kk < (cols-blockSize-1); kk += blockSize ) {
            int kend = kk + blockSize;
            
            // align each row until we're past the diagonal for this block
            for (size_t m = 0; m < blockSize; ++m) {
                for ( size_t j = kk+1+m ; j < kend; ++j ) {     // because we're flipping across the diagonal
                    std::swap( src[ (kk+m)*sRowStep+j ], src[ j*sRowStep+(kk+m) ] );
                }
            }
            
            // do full blocksize sweep in chunks
            size_t j;
            for ( j = kend; j < (cols-blockSize-1); j += blockSize ) {
                // copy rows rotated into temp
                for (size_t m = 0; m < blockSize; ++m) {
                    for (size_t n = 0; n < blockSize; ++n) {
                        tempBuffer[ m*blockSize + n ] = src[ (j+n)*sRowStep+(kk+m) ];
                    }
                }
                
                // copy cols rotated into rows
                for (size_t n = 0; n < blockSize; ++n) {
                    for (size_t m = 0; m < blockSize; ++m) {
                       src[ (j+n)*sRowStep+(kk+m) ] = src[ (kk+m)*sRowStep+(j+n)];
                    }
                }
                
                // copy temp into cols
                for (size_t m = 0; m < blockSize; ++m) {
                    for (size_t n = 0; n < blockSize; ++n) {
                       src[ (kk+m)*sRowStep+(j+n) ] = tempBuffer[ m*blockSize + n ];
                    }
                }
            }
            for ( ; j < cols; ++j ) {
                for (size_t m = 0; m < blockSize; ++m) {
                    std::swap( src[ (kk+m)*sRowStep+j ], src[ j*sRowStep+(kk+m) ] );
                }
            }
        }
        for ( ; kk < cols; ++kk ) {
            for ( size_t j = kk+1 ; j < cols; ++j ) {     // because we're flipping across the diagonal
                std::swap( src[ (kk)*sRowStep+j ], src[ j*sRowStep+(kk) ] );
            }
        }
    }
};

/******************************************************************************/

// assert( rows == cols );
// work in blocks
// use a temporary block instead of swap
// with inner loop order reversed to show cache effects (and that loop reordering is an important optimization)
// NOTE - optimal block size is different for various data sizes
template <typename T >
struct transpose_inplace8 {
    void operator()(T *src, size_t cols, int sRowStep) {
        const size_t blockSize = 8;
        T tempBuffer[ blockSize*blockSize ];
        size_t kk;
        for ( kk = 0; kk < (cols-blockSize-1); kk += blockSize ) {
            int kend = kk + blockSize;
            
            // align each row until we're past the diagonal for this block
            for (size_t m = 0; m < blockSize; ++m) {
                for ( size_t j = kk+1+m ; j < kend; ++j ) {     // because we're flipping across the diagonal
                    std::swap( src[ (kk+m)*sRowStep+j ], src[ j*sRowStep+(kk+m) ] );
                }
            }
            
            // do full blocksize sweep in chunks
            size_t j;
            for ( j = kend; j < (cols-blockSize-1); j += blockSize ) {
                // copy rows rotated into temp
                for (size_t n = 0; n < blockSize; ++n) {
                    for (size_t m = 0; m < blockSize; ++m) {
                        tempBuffer[ m*blockSize + n ] = src[ (j+n)*sRowStep+(kk+m) ];
                    }
                }
                
                // copy cols rotated into rows
                for (size_t m = 0; m < blockSize; ++m) {
                    for (size_t n = 0; n < blockSize; ++n) {
                       src[ (j+n)*sRowStep+(kk+m) ] = src[ (kk+m)*sRowStep+(j+n)];
                    }
                }
                
                // copy temp into cols
                for (size_t n = 0; n < blockSize; ++n) {
                    for (size_t m = 0; m < blockSize; ++m) {
                       src[ (kk+m)*sRowStep+(j+n) ] = tempBuffer[ m*blockSize + n ];
                    }
                }
            }
            for ( ; j < cols; ++j ) {
                for (size_t m = 0; m < blockSize; ++m) {
                    std::swap( src[ (kk+m)*sRowStep+j ], src[ j*sRowStep+(kk+m) ] );
                }
            }
        }
        for ( ; kk < cols; ++kk ) {
            for ( size_t j = kk+1 ; j < cols; ++j ) {     // because we're flipping across the diagonal
                std::swap( src[ (kk)*sRowStep+j ], src[ j*sRowStep+(kk) ] );
            }
        }
    }
};

/******************************************************************************/
/******************************************************************************/

// because of rotation, sRows -> dCols, sCols -> dRows
template <typename T >
struct rotate90CW_1 {
    void operator()(const T *src, T *dst, size_t rows, size_t cols, int sRowStep, int dRowStep) {
        for ( size_t k = 0; k < cols; ++k ) {
            for ( size_t j = 0 ; j < rows ; ++j ) {
                dst[ k*dRowStep+j ] = src[ (rows-1-j)*sRowStep + k ];
            }
        }
    }
};

/******************************************************************************/

// invert loops
template <typename T >
struct rotate90CW_2 {
    void operator()(const T *src, T *dst, size_t rows, size_t cols, int sRowStep, int dRowStep) {
        for ( size_t j = 0 ; j < rows ; ++j ) {
            for ( size_t k = 0; k < cols; ++k ) {
                dst[ k*dRowStep+j ] = src[ (rows-1-j)*sRowStep + k ];
            }
        }
    }
};

/******************************************************************************/

template <typename T >
struct rotate90CW_3 {
    void operator()(const T *src, T *dst, size_t rows, size_t cols, int sRowStep, int dRowStep) {
        for ( size_t k = 0; k < cols; ++k ) {
            for ( size_t j = 0 ; j < rows ; ++j ) {
                dst[ j ] = src[ (rows-1-j)*sRowStep ];
            }
            src += 1;
            dst += dRowStep;
        }
    }
};

/******************************************************************************/

// unroll one loop for slightly better cache usage
template <typename T >
struct rotate90CW_4 {
    void operator()(const T *src, T *dst, size_t rows, size_t cols, int sRowStep, int dRowStep) {
        size_t k;
        for ( k = 0; k < (cols-3); k += 4 ) {
            for ( size_t j = 0 ; j < rows ; ++j ) {
                dst[ 0*dRowStep+j ] = src[ (rows-1-j)*sRowStep + 0 ];
                dst[ 1*dRowStep+j ] = src[ (rows-1-j)*sRowStep + 1 ];
                dst[ 2*dRowStep+j ] = src[ (rows-1-j)*sRowStep + 2 ];
                dst[ 3*dRowStep+j ] = src[ (rows-1-j)*sRowStep + 3 ];
            }
            src += 4;
            dst += 4*dRowStep;
        }
        for ( ; k < cols; ++k ) {
            for ( size_t j = 0 ; j < rows ; ++j ) {
                dst[ j ] = src[ (rows-1-j)*sRowStep ];
            }
            src += 1;
            dst += dRowStep;
        }
    }
};

/******************************************************************************/

// unroll 2 loops for slightly better cache usage
template <typename T >
struct rotate90CW_5 {
    void operator()(const T *src, T *dst, size_t rows, size_t cols, int sRowStep, int dRowStep) {
        size_t j, k;
        for ( k = 0; k < (cols-3); k += 4 ) {
            for ( j = 0 ; j < (rows-3) ; j += 4 ) {
                dst[ 0*dRowStep+(j+0) ] = src[ (rows-1-(j+0))*sRowStep + 0 ];
                dst[ 0*dRowStep+(j+1) ] = src[ (rows-1-(j+1))*sRowStep + 0 ];
                dst[ 0*dRowStep+(j+2) ] = src[ (rows-1-(j+2))*sRowStep + 0 ];
                dst[ 0*dRowStep+(j+3) ] = src[ (rows-1-(j+3))*sRowStep + 0 ];
                
                dst[ 1*dRowStep+(j+0) ] = src[ (rows-1-(j+0))*sRowStep + 1 ];
                dst[ 1*dRowStep+(j+1) ] = src[ (rows-1-(j+1))*sRowStep + 1 ];
                dst[ 1*dRowStep+(j+2) ] = src[ (rows-1-(j+2))*sRowStep + 1 ];
                dst[ 1*dRowStep+(j+3) ] = src[ (rows-1-(j+3))*sRowStep + 1 ];
                
                dst[ 2*dRowStep+(j+0) ] = src[ (rows-1-(j+0))*sRowStep + 2 ];
                dst[ 2*dRowStep+(j+1) ] = src[ (rows-1-(j+1))*sRowStep + 2 ];
                dst[ 2*dRowStep+(j+2) ] = src[ (rows-1-(j+2))*sRowStep + 2 ];
                dst[ 2*dRowStep+(j+3) ] = src[ (rows-1-(j+3))*sRowStep + 2 ];
                
                dst[ 3*dRowStep+(j+0) ] = src[ (rows-1-(j+0))*sRowStep + 3 ];
                dst[ 3*dRowStep+(j+1) ] = src[ (rows-1-(j+1))*sRowStep + 3 ];
                dst[ 3*dRowStep+(j+2) ] = src[ (rows-1-(j+2))*sRowStep + 3 ];
                dst[ 3*dRowStep+(j+3) ] = src[ (rows-1-(j+3))*sRowStep + 3 ];
            }
            for ( ; j < rows ; ++j ) {
                dst[ 0*dRowStep+j ] = src[ (rows-1-j)*sRowStep + 0 ];
                dst[ 1*dRowStep+j ] = src[ (rows-1-j)*sRowStep + 1 ];
                dst[ 2*dRowStep+j ] = src[ (rows-1-j)*sRowStep + 2 ];
                dst[ 3*dRowStep+j ] = src[ (rows-1-j)*sRowStep + 3 ];
            }
            src += 4;
            dst += 4*dRowStep;
        }
        for ( ; k < cols; ++k ) {
            for ( size_t j = 0 ; j < rows ; ++j ) {
                dst[ j ] = src[ (rows-1-j)*sRowStep ];
            }
            src += 1;
            dst += dRowStep;
        }
    }
};

/******************************************************************************/

template <typename T >
struct rotate90CW_6 {
    void operator()(const T *src, T *dst, size_t rows, size_t cols, int sRowStep, int dRowStep) {
        transpose1<T>()( src, dst, rows, cols, sRowStep, dRowStep );
        flipHorizontal_inplace1<T>()( dst, cols, rows, dRowStep );
    }
};

/******************************************************************************/
/******************************************************************************/

// assert( rows == cols );
template <typename T >
struct rotate90CW_inplace1 {
    void operator()(T *src, size_t cols, int sRowStep) {
        for ( size_t k = 0; k <= (cols/2); ++k ) {
            for ( size_t j = k ; j < (cols-1-k) ; ++j ) {
                // 4 point rotation with temporaries
                auto sUL = src[ k*sRowStep+j ];
                auto sLL = src[ (cols-1-j)*sRowStep + k ];
                auto sUR = src[ j*sRowStep+(cols-1-k) ];
                auto sLR = src[ (cols-1-k)*sRowStep + (cols-1-j) ];
                
                src[ k*sRowStep+j ] = sLL;
                src[ (cols-1-j)*sRowStep + k ] = sLR;
                src[ j*sRowStep+(cols-1-k) ] = sUL;
                src[ (cols-1-k)*sRowStep + (cols-1-j) ] = sUR;
            }
        }
    }
};

/******************************************************************************/

// assert( rows == cols );
// invert loops (should make no difference in a 4 way rotationally symmetric algorithm)
template <typename T >
struct rotate90CW_inplace2 {
    void operator()(T *src, size_t cols, int sRowStep) {
        for ( size_t j = 0 ; j <= (cols/2); ++j ) {
            for ( size_t k = j; k < (cols-1-j); ++k ) {
                // 4 point rotation with temporaries
                auto sUL = src[ k*sRowStep+j ];
                auto sLL = src[ (cols-1-j)*sRowStep + k ];
                auto sUR = src[ j*sRowStep+(cols-1-k) ];
                auto sLR = src[ (cols-1-k)*sRowStep + (cols-1-j) ];
                
                src[ k*sRowStep+j ] = sLL;
                src[ (cols-1-j)*sRowStep + k ] = sLR;
                src[ j*sRowStep+(cols-1-k) ] = sUL;
                src[ (cols-1-k)*sRowStep + (cols-1-j) ] = sUR;
            }
        }
    }
};

/******************************************************************************/

// assert( rows == cols );
// unroll inner loop
template <typename T >
struct rotate90CW_inplace3 {
    void operator()(T *src, size_t cols, int sRowStep) {
        for ( size_t k = 0; k <= (cols/2); ++k ) {
            size_t j;
            for ( j = k ; j < (cols-1-k-1) ; j += 2 ) {
                // 4 point rotation with temporaries
                auto sUL = src[ k*sRowStep+(j+0) ];
                auto sLL = src[ (cols-1-(j+0))*sRowStep + k ];
                auto sUR = src[ (j+0)*sRowStep+(cols-1-k) ];
                auto sLR = src[ (cols-1-k)*sRowStep + (cols-1-(j+0)) ];
                
                src[ k*sRowStep+(j+0) ] = sLL;
                src[ (cols-1-(j+0))*sRowStep + k ] = sLR;
                src[ (j+0)*sRowStep+(cols-1-k) ] = sUL;
                src[ (cols-1-k)*sRowStep + (cols-1-(j+0)) ] = sUR;
                
                auto sUL1 = src[ k*sRowStep+(j+1) ];
                auto sLL1 = src[ (cols-1-(j+1))*sRowStep + k ];
                auto sUR1 = src[ (j+1)*sRowStep+(cols-1-k) ];
                auto sLR1 = src[ (cols-1-k)*sRowStep + (cols-1-(j+1)) ];
                
                src[ k*sRowStep+(j+1) ] = sLL1;
                src[ (cols-1-(j+1))*sRowStep + k ] = sLR1;
                src[ (j+1)*sRowStep+(cols-1-k) ] = sUL1;
                src[ (cols-1-k)*sRowStep + (cols-1-(j+1)) ] = sUR1;
            }
            for ( ; j < (cols-1-k) ; ++j ) {
                // 4 point rotation with temporaries
                auto sUL = src[ k*sRowStep+j ];
                auto sLL = src[ (cols-1-j)*sRowStep + k ];
                auto sUR = src[ j*sRowStep+(cols-1-k) ];
                auto sLR = src[ (cols-1-k)*sRowStep + (cols-1-j) ];
                
                src[ k*sRowStep+j ] = sLL;
                src[ (cols-1-j)*sRowStep + k ] = sLR;
                src[ j*sRowStep+(cols-1-k) ] = sUL;
                src[ (cols-1-k)*sRowStep + (cols-1-j) ] = sUR;
            }
        }
    }
};

/******************************************************************************/

// assert( rows == cols );
// unroll inner loop more
template <typename T >
struct rotate90CW_inplace4 {
    void operator()(T *src, size_t cols, int sRowStep) {
        for ( size_t k = 0; k <= (cols/2); ++k ) {
            size_t j;
            for ( j = k ; j < (cols-1-k-3) ; j += 4 ) {
                // 4 point rotation with temporaries
                auto sUL = src[ k*sRowStep+(j+0) ];
                auto sLL = src[ (cols-1-(j+0))*sRowStep + k ];
                auto sUR = src[ (j+0)*sRowStep+(cols-1-k) ];
                auto sLR = src[ (cols-1-k)*sRowStep + (cols-1-(j+0)) ];
                
                src[ k*sRowStep+(j+0) ] = sLL;
                src[ (cols-1-(j+0))*sRowStep + k ] = sLR;
                src[ (j+0)*sRowStep+(cols-1-k) ] = sUL;
                src[ (cols-1-k)*sRowStep + (cols-1-(j+0)) ] = sUR;
                
                auto sUL1 = src[ k*sRowStep+(j+1) ];
                auto sLL1 = src[ (cols-1-(j+1))*sRowStep + k ];
                auto sUR1 = src[ (j+1)*sRowStep+(cols-1-k) ];
                auto sLR1 = src[ (cols-1-k)*sRowStep + (cols-1-(j+1)) ];
                
                src[ k*sRowStep+(j+1) ] = sLL1;
                src[ (cols-1-(j+1))*sRowStep + k ] = sLR1;
                src[ (j+1)*sRowStep+(cols-1-k) ] = sUL1;
                src[ (cols-1-k)*sRowStep + (cols-1-(j+1)) ] = sUR1;
                
                auto sUL2 = src[ k*sRowStep+(j+2) ];
                auto sLL2 = src[ (cols-1-(j+2))*sRowStep + k ];
                auto sUR2 = src[ (j+2)*sRowStep+(cols-1-k) ];
                auto sLR2 = src[ (cols-1-k)*sRowStep + (cols-1-(j+2)) ];
                
                src[ k*sRowStep+(j+2) ] = sLL2;
                src[ (cols-1-(j+2))*sRowStep + k ] = sLR2;
                src[ (j+2)*sRowStep+(cols-1-k) ] = sUL2;
                src[ (cols-1-k)*sRowStep + (cols-1-(j+2)) ] = sUR2;
                
                auto sUL3 = src[ k*sRowStep+(j+3) ];
                auto sLL3 = src[ (cols-1-(j+3))*sRowStep + k ];
                auto sUR3 = src[ (j+3)*sRowStep+(cols-1-k) ];
                auto sLR3 = src[ (cols-1-k)*sRowStep + (cols-1-(j+3)) ];
                
                src[ k*sRowStep+(j+3) ] = sLL3;
                src[ (cols-1-(j+3))*sRowStep + k ] = sLR3;
                src[ (j+3)*sRowStep+(cols-1-k) ] = sUL3;
                src[ (cols-1-k)*sRowStep + (cols-1-(j+3)) ] = sUR3;
            }
            for ( ; j < (cols-1-k) ; ++j ) {
                // 4 point rotation with temporaries
                auto sUL = src[ k*sRowStep+j ];
                auto sLL = src[ (cols-1-j)*sRowStep + k ];
                auto sUR = src[ j*sRowStep+(cols-1-k) ];
                auto sLR = src[ (cols-1-k)*sRowStep + (cols-1-j) ];
                
                src[ k*sRowStep+j ] = sLL;
                src[ (cols-1-j)*sRowStep + k ] = sLR;
                src[ j*sRowStep+(cols-1-k) ] = sUL;
                src[ (cols-1-k)*sRowStep + (cols-1-j) ] = sUR;
            }
        }
    }
};

/******************************************************************************/

// assert( rows == cols );
// unroll inner loop more, and rearrange loads/stores
// NOTE: bad instruction scheduling can kill performance!
template <typename T >
struct rotate90CW_inplace5 {
    void operator()(T *src, size_t cols, int sRowStep) {
        for ( size_t k = 0; k <= (cols/2); ++k ) {
            size_t j;
            for ( j = k ; j < (cols-1-k-3) ; j += 4 ) {
                // 4 point rotation with temporaries
                size_t tempULindex = k*sRowStep+(j+0);
                auto sUL  = src[ tempULindex+0 ];
                auto sUL1 = src[ tempULindex+1 ];
                auto sUL2 = src[ tempULindex+2 ];
                auto sUL3 = src[ tempULindex+3 ];
                
                size_t tempLRindex = (cols-1-k)*sRowStep + (cols-1-(j+3));
                auto sLR3 = src[ tempLRindex+0 ];
                auto sLR2 = src[ tempLRindex+1 ];
                auto sLR1 = src[ tempLRindex+2 ];
                auto sLR  = src[ tempLRindex+3 ];
                
                size_t tempLLindex = (cols-1-(j+0))*sRowStep + k;
                auto sLL  = src[ tempLLindex - 0*sRowStep ];
                auto sLL1 = src[ tempLLindex - 1*sRowStep ];
                auto sLL2 = src[ tempLLindex - 2*sRowStep ];
                auto sLL3 = src[ tempLLindex - 3*sRowStep ];
                
                size_t tempURindex = (j+0)*sRowStep+(cols-1-k);
                auto sUR  = src[ tempURindex + 0*sRowStep ];
                auto sUR1 = src[ tempURindex + 1*sRowStep];
                auto sUR2 = src[ tempURindex + 2*sRowStep ];
                auto sUR3 = src[ tempURindex + 3*sRowStep ];
                
                src[ tempULindex+0 ] = sLL;
                src[ tempULindex+1 ] = sLL1;
                src[ tempULindex+2 ] = sLL2;
                src[ tempULindex+3 ] = sLL3;
                
                src[ tempLRindex+0 ] = sUR3;
                src[ tempLRindex+1 ] = sUR2;
                src[ tempLRindex+2 ] = sUR1;
                src[ tempLRindex+3 ] = sUR;
                
                src[ tempLLindex - 0*sRowStep ] = sLR;
                src[ tempLLindex - 1*sRowStep ] = sLR1;
                src[ tempLLindex - 2*sRowStep ] = sLR2;
                src[ tempLLindex - 3*sRowStep] = sLR3;
                
                src[ tempURindex + 0*sRowStep ] = sUL;
                src[ tempURindex + 1*sRowStep ] = sUL1;
                src[ tempURindex + 2*sRowStep ] = sUL2;
                src[ tempURindex + 3*sRowStep ] = sUL3;

            }
            for ( ; j < (cols-1-k) ; ++j ) {
                // 4 point rotation with temporaries
                auto sUL = src[ k*sRowStep+j ];
                auto sLL = src[ (cols-1-j)*sRowStep + k ];
                auto sUR = src[ j*sRowStep+(cols-1-k) ];
                auto sLR = src[ (cols-1-k)*sRowStep + (cols-1-j) ];
                
                src[ k*sRowStep+j ] = sLL;
                src[ (cols-1-j)*sRowStep + k ] = sLR;
                src[ j*sRowStep+(cols-1-k) ] = sUL;
                src[ (cols-1-k)*sRowStep + (cols-1-j) ] = sUR;
            }
        }
    }
};

/******************************************************************************/

template <typename T >
struct rotate90CW_inplace6 {
    void operator()(T *src, size_t cols, int sRowStep) {
        transpose_inplace1<T>()( src, cols, sRowStep );
        flipHorizontal_inplace1<T>()( src, cols, cols, sRowStep );
    }
};

/******************************************************************************/

// assert( rows == cols );
template <typename T >
struct rotate90CW_inplace7 {
    void operator()(T *src, size_t cols, int sRowStep) {
        for ( size_t k = 0; k <= (cols/2); ++k ) {
            for ( size_t j = k ; j < (cols-1-k) ; ++j ) {
                // 4 point rotation with 3 swaps
                std::swap( src[ k*sRowStep+j ], src[ j*sRowStep+(cols-1-k) ] ); // UL <-> UR
                std::swap( src[ (cols-1-j)*sRowStep + k ], src[ (cols-1-k)*sRowStep + (cols-1-j) ] ); // LL <-> LR
                std::swap( src[ k*sRowStep+j ], src[ (cols-1-k)*sRowStep + (cols-1-j) ]); // UL <-> LR
            }
        }
    }
};

/******************************************************************************/
/******************************************************************************/

// because of rotation, sRows -> dCols, sCols -> dRows
template <typename T >
struct rotate90CCW_1 {
    void operator()(const T *src, T *dst, size_t rows, size_t cols, int sRowStep, int dRowStep) {
        for ( size_t k = 0; k < cols; ++k ) {
            for ( size_t j = 0 ; j < rows ; ++j ) {
                dst[ k*dRowStep+j ] = src[ j*sRowStep + (cols-1-k) ];
            }
        }
    }
};

/******************************************************************************/

// invert loops
template <typename T >
struct rotate90CCW_2 {
    void operator()(const T *src, T *dst, size_t rows, size_t cols, int sRowStep, int dRowStep) {
        for ( size_t j = 0 ; j < rows ; ++j ) {
            for ( size_t k = 0; k < cols; ++k ) {
                dst[ k*dRowStep+j ] = src[ j*sRowStep + (cols-1-k) ];
            }
        }
    }
};

/******************************************************************************/

template <typename T >
struct rotate90CCW_3 {
    void operator()(const T *src, T *dst, size_t rows, size_t cols, int sRowStep, int dRowStep) {
        auto srcTemp = src + cols-1;
        for ( size_t k = 0; k < cols; ++k ) {
            for ( size_t j = 0 ; j < rows ; ++j ) {
                dst[ j ] = srcTemp[ j*sRowStep ];
            }
            srcTemp -= 1;
            dst += dRowStep;
        }
    }
};

/******************************************************************************/

// unroll one loop for slightly better cache usage
template <typename T >
struct rotate90CCW_4 {
    void operator()(const T *src, T *dst, size_t rows, size_t cols, int sRowStep, int dRowStep) {
        auto srcTemp = src + cols-1;
        size_t k;
        for ( k = 0; k < (cols-3); k += 4 ) {
            for ( size_t j = 0 ; j < rows ; ++j ) {
                dst[ dRowStep*0 + j ] = srcTemp[ j*sRowStep - 0 ];
                dst[ dRowStep*1 + j ] = srcTemp[ j*sRowStep - 1 ];
                dst[ dRowStep*2 + j ] = srcTemp[ j*sRowStep - 2 ];
                dst[ dRowStep*3 + j ] = srcTemp[ j*sRowStep - 3 ];
            }
            srcTemp -= 4;
            dst += 4*dRowStep;
        }
        for ( ; k < cols; ++k ) {
            for ( size_t j = 0 ; j < rows ; ++j ) {
                dst[ j ] = srcTemp[ j*sRowStep ];
            }
            srcTemp -= 1;
            dst += dRowStep;
        }
    }
};

/******************************************************************************/

// unroll 2 loops for slightly better cache usage
template <typename T >
struct rotate90CCW_5 {
    void operator()(const T *src, T *dst, size_t rows, size_t cols, int sRowStep, int dRowStep) {
        auto srcTemp = src + cols-1;
        size_t j, k;
        for ( k = 0; k < (cols-3); k += 4 ) {
            for ( j = 0 ; j < (rows-3) ; j += 4 ) {
                dst[ dRowStep*0 + (j+0) ] = srcTemp[ (j+0)*sRowStep - 0 ];
                dst[ dRowStep*0 + (j+1) ] = srcTemp[ (j+1)*sRowStep - 0 ];
                dst[ dRowStep*0 + (j+2) ] = srcTemp[ (j+2)*sRowStep - 0 ];
                dst[ dRowStep*0 + (j+3) ] = srcTemp[ (j+3)*sRowStep - 0 ];
                
                dst[ dRowStep*1 + (j+0) ] = srcTemp[ (j+0)*sRowStep - 1 ];
                dst[ dRowStep*1 + (j+1) ] = srcTemp[ (j+1)*sRowStep - 1 ];
                dst[ dRowStep*1 + (j+2) ] = srcTemp[ (j+2)*sRowStep - 1 ];
                dst[ dRowStep*1 + (j+3) ] = srcTemp[ (j+3)*sRowStep - 1 ];
                
                dst[ dRowStep*2 + (j+0) ] = srcTemp[ (j+0)*sRowStep - 2 ];
                dst[ dRowStep*2 + (j+1) ] = srcTemp[ (j+1)*sRowStep - 2 ];
                dst[ dRowStep*2 + (j+2) ] = srcTemp[ (j+2)*sRowStep - 2 ];
                dst[ dRowStep*2 + (j+3) ] = srcTemp[ (j+3)*sRowStep - 2 ];
                
                dst[ dRowStep*3 + (j+0) ] = srcTemp[ (j+0)*sRowStep - 3 ];
                dst[ dRowStep*3 + (j+1) ] = srcTemp[ (j+1)*sRowStep - 3 ];
                dst[ dRowStep*3 + (j+2) ] = srcTemp[ (j+2)*sRowStep - 3 ];
                dst[ dRowStep*3 + (j+3) ] = srcTemp[ (j+3)*sRowStep - 3 ];
            }
            for ( ; j < rows ; ++j ) {
                dst[ dRowStep*0 + j ] = srcTemp[ j*sRowStep - 0 ];
                dst[ dRowStep*1 + j ] = srcTemp[ j*sRowStep - 1 ];
                dst[ dRowStep*2 + j ] = srcTemp[ j*sRowStep - 2 ];
                dst[ dRowStep*3 + j ] = srcTemp[ j*sRowStep - 3 ];
            }
            srcTemp -= 4;
            dst += 4*dRowStep;
        }
        for ( ; k < cols; ++k ) {
            for ( j = 0 ; j < rows ; ++j ) {
                dst[ j ] = srcTemp[ j*sRowStep ];
            }
            srcTemp -= 1;
            dst += dRowStep;
        }
    }
};

/******************************************************************************/

template <typename T >
struct rotate90CCW_6 {
    void operator()(const T *src, T *dst, size_t rows, size_t cols, int sRowStep, int dRowStep) {
        transpose1<T>()( src, dst, rows, cols, sRowStep, dRowStep );
        flipVertical_inplace1<T>()( dst, cols, rows, dRowStep );
    }
};

/******************************************************************************/
/******************************************************************************/

// assert( rows == cols );
template <typename T >
struct rotate90CCW_inplace1 {
    void operator()(T *src, size_t cols, int sRowStep) {
        for ( size_t k = 0; k <= (cols/2); ++k ) {
            for ( size_t j = k ; j < (cols-1-k) ; ++j ) {
                // 4 point rotation with temporaries
                auto sUL = src[ k*sRowStep+j ];
                auto sLL = src[ (cols-1-j)*sRowStep + k ];
                auto sUR = src[ j*sRowStep+(cols-1-k) ];
                auto sLR = src[ (cols-1-k)*sRowStep + (cols-1-j) ];
                
                src[ k*sRowStep+j ] = sUR;
                src[ (cols-1-j)*sRowStep + k ] = sUL;
                src[ j*sRowStep+(cols-1-k) ] = sLR;
                src[ (cols-1-k)*sRowStep + (cols-1-j) ] = sLL;
            }
        }
    }
};

/******************************************************************************/

// assert( rows == cols );
// invert loops (should make no difference in a 4 way rotationally symmetric algorithm)
template <typename T >
struct rotate90CCW_inplace2 {
    void operator()(T *src, size_t cols, int sRowStep) {
        for ( size_t j = 0 ; j <= (cols/2); ++j ) {
            for ( size_t k = j; k < (cols-1-j); ++k ) {
                // 4 point rotation with temporaries
                auto sUL = src[ k*sRowStep+j ];
                auto sLL = src[ (cols-1-j)*sRowStep + k ];
                auto sUR = src[ j*sRowStep+(cols-1-k) ];
                auto sLR = src[ (cols-1-k)*sRowStep + (cols-1-j) ];
                
                src[ k*sRowStep+j ] = sUR;
                src[ (cols-1-j)*sRowStep + k ] = sUL;
                src[ j*sRowStep+(cols-1-k) ] = sLR;
                src[ (cols-1-k)*sRowStep + (cols-1-j) ] = sLL;
            }
        }
    }
};

/******************************************************************************/

// assert( rows == cols );
// unroll inner loop
template <typename T >
struct rotate90CCW_inplace3 {
    void operator()(T *src, size_t cols, int sRowStep) {
        for ( size_t k = 0; k <= (cols/2); ++k ) {
            size_t j;
            for ( j = k ; j < (cols-1-k-1) ; j += 2 ) {
                // 4 point rotation with temporaries
                auto sUL = src[ k*sRowStep+(j+0) ];
                auto sLL = src[ (cols-1-(j+0))*sRowStep + k ];
                auto sUR = src[ (j+0)*sRowStep+(cols-1-k) ];
                auto sLR = src[ (cols-1-k)*sRowStep + (cols-1-(j+0)) ];
                
                src[ k*sRowStep+(j+0) ] = sUR;
                src[ (cols-1-(j+0))*sRowStep + k ] = sUL;
                src[ (j+0)*sRowStep+(cols-1-k) ] = sLR;
                src[ (cols-1-k)*sRowStep + (cols-1-(j+0)) ] = sLL;
                
                auto sUL1 = src[ k*sRowStep+(j+1) ];
                auto sLL1 = src[ (cols-1-(j+1))*sRowStep + k ];
                auto sUR1 = src[ (j+1)*sRowStep+(cols-1-k) ];
                auto sLR1 = src[ (cols-1-k)*sRowStep + (cols-1-(j+1)) ];
                
                src[ k*sRowStep+(j+1) ] = sUR1;
                src[ (cols-1-(j+1))*sRowStep + k ] = sUL1;
                src[ (j+1)*sRowStep+(cols-1-k) ] = sLR1;
                src[ (cols-1-k)*sRowStep + (cols-1-(j+1)) ] = sLL1;
            }
            for ( ; j < (cols-1-k) ; ++j ) {
                // 4 point rotation with temporaries
                auto sUL = src[ k*sRowStep+j ];
                auto sLL = src[ (cols-1-j)*sRowStep + k ];
                auto sUR = src[ j*sRowStep+(cols-1-k) ];
                auto sLR = src[ (cols-1-k)*sRowStep + (cols-1-j) ];
                
                src[ k*sRowStep+j ] = sUR;
                src[ (cols-1-j)*sRowStep + k ] = sUL;
                src[ j*sRowStep+(cols-1-k) ] = sLR;
                src[ (cols-1-k)*sRowStep + (cols-1-j) ] = sLL;
            }
        }
    }
};

/******************************************************************************/

// assert( rows == cols );
// unroll inner loop more
template <typename T >
struct rotate90CCW_inplace4 {
    void operator()(T *src, size_t cols, int sRowStep) {
        for ( size_t k = 0; k <= (cols/2); ++k ) {
            size_t j;
            for ( j = k ; j < (cols-1-k-3) ; j += 4 ) {
                // 4 point rotation with temporaries
                auto sUL = src[ k*sRowStep+(j+0) ];
                auto sLL = src[ (cols-1-(j+0))*sRowStep + k ];
                auto sUR = src[ (j+0)*sRowStep+(cols-1-k) ];
                auto sLR = src[ (cols-1-k)*sRowStep + (cols-1-(j+0)) ];
                
                src[ k*sRowStep+(j+0) ] = sUR;
                src[ (cols-1-(j+0))*sRowStep + k ] = sUL;
                src[ (j+0)*sRowStep+(cols-1-k) ] = sLR;
                src[ (cols-1-k)*sRowStep + (cols-1-(j+0)) ] = sLL;
                
                auto sUL1 = src[ k*sRowStep+(j+1) ];
                auto sLL1 = src[ (cols-1-(j+1))*sRowStep + k ];
                auto sUR1 = src[ (j+1)*sRowStep+(cols-1-k) ];
                auto sLR1 = src[ (cols-1-k)*sRowStep + (cols-1-(j+1)) ];
                
                src[ k*sRowStep+(j+1) ] = sUR1;
                src[ (cols-1-(j+1))*sRowStep + k ] = sUL1;
                src[ (j+1)*sRowStep+(cols-1-k) ] = sLR1;
                src[ (cols-1-k)*sRowStep + (cols-1-(j+1)) ] = sLL1;
                
                auto sUL2 = src[ k*sRowStep+(j+2) ];
                auto sLL2 = src[ (cols-1-(j+2))*sRowStep + k ];
                auto sUR2 = src[ (j+2)*sRowStep+(cols-1-k) ];
                auto sLR2 = src[ (cols-1-k)*sRowStep + (cols-1-(j+2)) ];
                
                src[ k*sRowStep+(j+2) ] = sUR2;
                src[ (cols-1-(j+2))*sRowStep + k ] = sUL2;
                src[ (j+2)*sRowStep+(cols-1-k) ] = sLR2;
                src[ (cols-1-k)*sRowStep + (cols-1-(j+2)) ] = sLL2;
                
                auto sUL3 = src[ k*sRowStep+(j+3) ];
                auto sLL3 = src[ (cols-1-(j+3))*sRowStep + k ];
                auto sUR3 = src[ (j+3)*sRowStep+(cols-1-k) ];
                auto sLR3 = src[ (cols-1-k)*sRowStep + (cols-1-(j+3)) ];
                
                src[ k*sRowStep+(j+3) ] = sUR3;
                src[ (cols-1-(j+3))*sRowStep + k ] = sUL3;
                src[ (j+3)*sRowStep+(cols-1-k) ] = sLR3;
                src[ (cols-1-k)*sRowStep + (cols-1-(j+3)) ] = sLL3;
            }
            for ( ; j < (cols-1-k) ; ++j ) {
                // 4 point rotation with temporaries
                auto sUL = src[ k*sRowStep+j ];
                auto sLL = src[ (cols-1-j)*sRowStep + k ];
                auto sUR = src[ j*sRowStep+(cols-1-k) ];
                auto sLR = src[ (cols-1-k)*sRowStep + (cols-1-j) ];
                
                src[ k*sRowStep+j ] = sUR;
                src[ (cols-1-j)*sRowStep + k ] = sUL;
                src[ j*sRowStep+(cols-1-k) ] = sLR;
                src[ (cols-1-k)*sRowStep + (cols-1-j) ] = sLL;
            }
        }
    }
};

/******************************************************************************/

template <typename T >
struct rotate90CCW_inplace5 {
    void operator()(T *src, size_t cols, int sRowStep) {
        transpose_inplace1<T>()( src, cols, sRowStep );
        flipVertical_inplace1<T>()( src, cols, cols, sRowStep );
    }
};

/******************************************************************************/

// assert( rows == cols );
template <typename T >
struct rotate90CCW_inplace6 {
    void operator()(T *src, size_t cols, int sRowStep) {
        for ( size_t k = 0; k <= (cols/2); ++k ) {
            for ( size_t j = k ; j < (cols-1-k) ; ++j ) {
                // 4 point rotation with 3 swaps
                std::swap( src[ k*sRowStep+j ], src[ j*sRowStep+(cols-1-k) ] ); // UL <-> UR
                std::swap( src[ (cols-1-j)*sRowStep + k ], src[ (cols-1-k)*sRowStep + (cols-1-j) ] ); // LL <-> LR
                std::swap( src[ j*sRowStep+(cols-1-k) ] , src[ (cols-1-j)*sRowStep + k ] ); // UR <-> LL
            }
        }
    }
};

/******************************************************************************/
/******************************************************************************/

// memcpy with flips/rotates
template <typename T>
void StepCopyBlock(const T *src, T *dst, size_t rows, size_t cols,
                    int sRowStep, int sColStep, int dRowStep, int dColStep ) {
    for (int j = 0 ; j < rows ; ++j ) {
        for (int k = 0; k < cols; ++k ) {
            dst[ dRowStep*j + dColStep*k ] = src[sRowStep*j + sColStep*k ];
        }
    }
}

/******************************************************************************/

// memcmp with flips/rotates
template <typename T>
bool StepCompareBlock(const T *src, const T *dst, size_t rows, size_t cols,
                    int sRowStep, int sColStep, int dRowStep, int dColStep ) {
    for (int j = 0 ; j < rows ; ++j ) {
        for (int k = 0; k < cols; ++k ) {
            auto expected = src[ sRowStep*j + sColStep*k ];
            auto result = dst[ dRowStep*j + dColStep*k ];
            if ( result != expected ) {
                std::cout << "Matrix failed to match at " << j << ", " << k << " ( " << std::hex << expected << " : " << result << " )\n";
                return true;
            }
        }
    }
    return false;
}

/******************************************************************************/
/******************************************************************************/

static std::deque<std::string> gLabels;

template <typename T, typename MM >
void test_flipH(const T *src, T *dst, size_t rows, size_t cols, MM flipper, const std::string label) {
    int sRowStep = cols;
    int dRowStep = cols;
    int sColStep = -1;
    int dColStep = 1;

    ::fill( dst, dst+(rows*cols), T(0) );

    start_timer();

    for(int i = 0; i < iterations; ++i) {
        flipper( src, dst, rows, cols, sRowStep, dRowStep );
    }
    
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );

    // and verify the result
    if (StepCompareBlock(src+(cols-1),dst,rows,cols,sRowStep,sColStep,dRowStep,dColStep) )  {
        printf("test %s failed\n", label.c_str() );
    }
}

/******************************************************************************/

template <typename T, typename MM >
void test_flipH_inplace( const T *src, T *dst, size_t rows, size_t cols, MM flipper, const std::string label) {
    int sRowStep = cols;
    int dRowStep = cols;
    int sColStep = -1;
    int dColStep = 1;

    ::copy( src, src+(rows*cols), dst );
    
    auto iter = iterations | 1; // so we have an odd number of iterations, matching a single flip of the source

    start_timer();

    for(int i = 0; i < iter; ++i) {
        flipper( dst, rows, cols, sRowStep );
    }
    
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );

    // and verify the result
    if (StepCompareBlock(src+(cols-1),dst,rows,cols,sRowStep,sColStep,dRowStep,dColStep) )  {
        printf("test %s failed\n", label.c_str() );
    }
}

/******************************************************************************/

template <typename T, typename MM >
void test_flipV(const T *src, T *dst, size_t rows, size_t cols, MM flipper, const std::string label) {
    int sRowStep = cols;
    int dRowStep = cols;
    int sColStep = 1;
    int dColStep = 1;

    ::fill( dst, dst+(rows*cols), T(0) );

    start_timer();

    for(int i = 0; i < iterations; ++i) {
        flipper( src, dst, rows, cols, sRowStep, dRowStep );
    }
    
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );

    // and verify the result
    if (StepCompareBlock(src+(rows-1)*sRowStep,dst,rows,cols,-sRowStep,sColStep,dRowStep,dColStep) )  {
        printf("test %s failed\n", label.c_str() );
    }
}

/******************************************************************************/

template <typename T, typename MM >
void test_flipV_inplace(const T *src, T *dst, size_t rows, size_t cols, MM flipper, const std::string label) {
    int sRowStep = cols;
    int dRowStep = cols;
    int sColStep = 1;
    int dColStep = 1;

    ::copy( src, src+(rows*cols), dst );
    
    auto iter = iterations | 1; // so we have an odd number of iterations, matching a single flip of the source

    start_timer();

    for(int i = 0; i < iter; ++i) {
        flipper( dst, rows, cols, cols );
    }
    
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );

    // and verify the result
    if (StepCompareBlock(src+(rows-1)*sRowStep,dst,rows,cols,-sRowStep,sColStep,dRowStep,dColStep) )  {
        printf("test %s failed\n", label.c_str() );
    }
}

/******************************************************************************/

template <typename T, typename MM >
void test_flip180(const T *src, T *dst, size_t rows, size_t cols, MM flipper, const std::string label) {
    int sRowStep = cols;
    int dRowStep = cols;
    int sColStep = -1;
    int dColStep = 1;

    ::fill( dst, dst+(rows*cols), T(0) );

    start_timer();

    for(int i = 0; i < iterations; ++i) {
        flipper( src, dst, rows, cols, sRowStep, dRowStep );
    }
    
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );

    // and verify the result
    if (StepCompareBlock(src+(rows-1)*sRowStep+(cols-1),dst,rows,cols,-sRowStep,sColStep,dRowStep,dColStep) )  {
        printf("test %s failed\n", label.c_str() );
    }
}

/******************************************************************************/

template <typename T, typename MM >
void test_flip180_inplace(const T *src, T *dst, size_t rows, size_t cols, MM flipper, const std::string label) {
    int sRowStep = cols;
    int dRowStep = cols;
    int sColStep = -1;
    int dColStep = 1;

    ::copy( src, src+(rows*cols), dst );
    
    auto iter = iterations | 1; // so we have an odd number of iterations, matching a single flip of the source

    start_timer();

    for(int i = 0; i < iter; ++i) {
        flipper( dst, rows, cols, cols );
    }
    
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );

    // and verify the result
    if (StepCompareBlock(src+(rows-1)*sRowStep+(cols-1),dst,rows,cols,-sRowStep,sColStep,dRowStep,dColStep) )  {
        printf("test %s failed\n", label.c_str() );
    }
}

/******************************************************************************/

template <typename T, typename MM >
void test_transpose(const T *src, T *dst, size_t rows, size_t cols, MM flipper, const std::string label) {
    int sRowStep = 1;
    int dRowStep = rows;
    int sColStep = cols;
    int dColStep = 1;

    ::fill( dst, dst+(rows*cols), T(0) );

    start_timer();

    for(int i = 0; i < iterations; ++i) {
        flipper( src, dst, rows, cols, cols, rows );
    }
    
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );

    // and verify the result
    if (StepCompareBlock(src,dst,cols,rows,sRowStep,sColStep,dRowStep,dColStep) )  {
        printf("test %s failed\n", label.c_str() );
    }
}

/******************************************************************************/

template <typename T, typename MM >
void test_transpose_inplace(const T *src, T *dst, size_t rows, size_t cols, MM flipper, const std::string label) {
    int sRowStep = 1;
    int dRowStep = rows;
    int sColStep = cols;
    int dColStep = 1;
    
    assert( rows == cols );

    ::copy( src, src+(rows*cols), dst );
    
    auto iter = iterations | 1; // so we have an odd number of iterations, matching a single flip of the source

    start_timer();

    for(int i = 0; i < iter; ++i) {
        flipper( dst, cols, cols );
    }
    
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );

    // and verify the result
    if (StepCompareBlock(src,dst,cols,rows,sRowStep,sColStep,dRowStep,dColStep) )  {
        printf("test %s failed\n", label.c_str() );
    }
}

/******************************************************************************/

template <typename T, typename MM >
void test_rotate90CW(const T *src, T *dst, size_t rows, size_t cols, MM flipper, const std::string label) {
    int sRowStep = 1;
    int dRowStep = rows;
    int sColStep = -cols;
    int dColStep = 1;

    ::fill( dst, dst+(rows*cols), T(0) );

    start_timer();
    
    for(int i = 0; i < iterations; ++i) {
        flipper( src, dst, rows, cols, cols, rows );
    }
    
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );

    // and verify the result
    if (StepCompareBlock(src+(rows-1)*cols,dst,cols,rows,sRowStep,sColStep,dRowStep,dColStep) )  {
        printf("test %s failed\n", label.c_str() );
    }
}

/******************************************************************************/

template <typename T, typename MM >
void test_rotate90CW_inplace(const T *src, T *dst, size_t rows, size_t cols, MM flipper, const std::string label) {
    int sRowStep = 1;
    int dRowStep = rows;
    int sColStep = -cols;
    int dColStep = 1;
    
    assert( rows == cols );

    ::copy( src, src+(rows*cols), dst );
    
    auto iter = ((iterations+3) & ~3) + 1; // modulo 4 to complete cycle, plus one to compare against single rotate

    start_timer();

    for(int i = 0; i < iter; ++i) {
        flipper( dst, cols, cols );
    }
    
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );

    // and verify the result
    if (StepCompareBlock(src+(rows-1)*cols,dst,cols,rows,sRowStep,sColStep,dRowStep,dColStep) )  {
        printf("test %s failed\n", label.c_str() );
    }
}

/******************************************************************************/

template <typename T, typename MM >
void test_rotate90CCW(const T *src, T *dst, size_t rows, size_t cols, MM flipper, const std::string label) {
    int sRowStep = -1;
    int dRowStep = rows;
    int sColStep = cols;
    int dColStep = 1;

    ::fill( dst, dst+(rows*cols), T(0) );
    
    start_timer();

    for(int i = 0; i < iterations; ++i) {
        flipper( src, dst, rows, cols, cols, rows );
    }
    
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );

    // and verify the result
    if (StepCompareBlock(src+(cols-1),dst,cols,rows,sRowStep,sColStep,dRowStep,dColStep) )  {
        printf("test %s failed\n", label.c_str() );
    }
}

/******************************************************************************/

template <typename T, typename MM >
void test_rotate90CCW_inplace(const T *src, T *dst, size_t rows, size_t cols, MM flipper, const std::string label) {
    int sRowStep = -1;
    int dRowStep = rows;
    int sColStep = cols;
    int dColStep = 1;
    
    assert( rows == cols );

    ::copy( src, src+(rows*cols), dst );
    
    auto iter = ((iterations+3) & ~3) + 1; // modulo 4 to complete cycle, plus one to compare against single rotate

    start_timer();

    for(int i = 0; i < iter; ++i) {
        flipper( dst, cols, cols );
    }
    
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );

    // and verify the result
    if (StepCompareBlock(src+(cols-1),dst,cols,rows,sRowStep,sColStep,dRowStep,dColStep) )  {
        printf("test %s failed\n", label.c_str() );
    }
}

/******************************************************************************/
/******************************************************************************/

template< typename T>
void TestOneType()
{
    std::string myTypeName( getTypeName<T>() );
    
    gLabels.clear();
    
    std::unique_ptr<T> dX( new T[HEIGHT * WIDTH] );
    std::unique_ptr<T> dY( new T[HEIGHT * WIDTH] );
    T *dataX = dX.get();
    T *dataY = dY.get();

    scrand( init_value );
    ::fill_random( dataX, dataX+(HEIGHT*WIDTH) );
    ::fill_random( dataY, dataY+(HEIGHT*WIDTH) );

    auto base_iterations = iterations;
    

    // First the copy versions

    test_flipH( dataX, dataY, HEIGHT, WIDTH, flipHorizontal1<T>(), myTypeName + " matrix flipHorizontal1" );
    test_flipH( dataX, dataY, HEIGHT, WIDTH, flipHorizontal2<T>(), myTypeName + " matrix flipHorizontal2" );
    test_flipH( dataX, dataY, HEIGHT, WIDTH, flipHorizontal3<T>(), myTypeName + " matrix flipHorizontal3" );
    test_flipH( dataX, dataY, HEIGHT, WIDTH, flipHorizontal4<T>(), myTypeName + " matrix flipHorizontal4" );

    std::string temp1( myTypeName + " matrix flipHorizontal" );
    summarize( temp1.c_str(), (HEIGHT*WIDTH), iterations, kDontShowGMeans, kDontShowPenalty );
    
    
    test_flipV( dataX, dataY, HEIGHT, WIDTH, flipVertical1<T>(), myTypeName + " matrix flipVertical1" );
    test_flipV( dataX, dataY, HEIGHT, WIDTH, flipVertical2<T>(), myTypeName + " matrix flipVertical2" );
    test_flipV( dataX, dataY, HEIGHT, WIDTH, flipVertical3<T>(), myTypeName + " matrix flipVertical3" );
    test_flipV( dataX, dataY, HEIGHT, WIDTH, flipVertical4<T>(), myTypeName + " matrix flipVertical4" );
    test_flipV( dataX, dataY, HEIGHT, WIDTH, flipVertical5<T>(), myTypeName + " matrix flipVertical5" );

    std::string temp2( myTypeName + " matrix flipVertical" );
    summarize( temp2.c_str(), (HEIGHT*WIDTH), iterations, kDontShowGMeans, kDontShowPenalty );
    
    
    test_flip180( dataX, dataY, HEIGHT, WIDTH, flip180_1<T>(), myTypeName + " matrix flip180_1" );
    test_flip180( dataX, dataY, HEIGHT, WIDTH, flip180_2<T>(), myTypeName + " matrix flip180_2" );
    test_flip180( dataX, dataY, HEIGHT, WIDTH, flip180_3<T>(), myTypeName + " matrix flip180_3" );
    test_flip180( dataX, dataY, HEIGHT, WIDTH, flip180_4<T>(), myTypeName + " matrix flip180_4" );
    test_flip180( dataX, dataY, HEIGHT, WIDTH, flip180_5<T>(), myTypeName + " matrix flip180_5" );

    std::string temp3( myTypeName + " matrix flip180" );
    summarize( temp3.c_str(), (HEIGHT*WIDTH), iterations, kDontShowGMeans, kDontShowPenalty );


    iterations /= 6;    // rotations are slower (don't vectorize well)
    
    test_transpose( dataX, dataY, HEIGHT, WIDTH, transpose1<T>(), myTypeName + " matrix transpose1" );
    test_transpose( dataX, dataY, HEIGHT, WIDTH, transpose2<T>(), myTypeName + " matrix transpose2" );
    test_transpose( dataX, dataY, HEIGHT, WIDTH, transpose3<T>(), myTypeName + " matrix transpose3" );
    test_transpose( dataX, dataY, HEIGHT, WIDTH, transpose4<T>(), myTypeName + " matrix transpose4" );
    test_transpose( dataX, dataY, HEIGHT, WIDTH, transpose5<T>(), myTypeName + " matrix transpose5" );
    test_transpose( dataX, dataY, HEIGHT, WIDTH, transpose6<T>(), myTypeName + " matrix transpose6" );
    test_transpose( dataX, dataY, HEIGHT, WIDTH, transpose7<T>(), myTypeName + " matrix transpose7" );
    test_transpose( dataX, dataY, HEIGHT, WIDTH, transpose8<T>(), myTypeName + " matrix transpose8" );
    test_transpose( dataX, dataY, HEIGHT, WIDTH, transpose9<T>(), myTypeName + " matrix transpose9" );

    std::string temp4( myTypeName + " matrix transpose" );
    summarize( temp4.c_str(), (HEIGHT*WIDTH), iterations, kDontShowGMeans, kDontShowPenalty );


    test_rotate90CW( dataX, dataY, HEIGHT, WIDTH, rotate90CW_1<T>(), myTypeName + " matrix rotate90CW_1" );
    test_rotate90CW( dataX, dataY, HEIGHT, WIDTH, rotate90CW_2<T>(), myTypeName + " matrix rotate90CW_2" );
    test_rotate90CW( dataX, dataY, HEIGHT, WIDTH, rotate90CW_3<T>(), myTypeName + " matrix rotate90CW_3" );
    test_rotate90CW( dataX, dataY, HEIGHT, WIDTH, rotate90CW_4<T>(), myTypeName + " matrix rotate90CW_4" );
    test_rotate90CW( dataX, dataY, HEIGHT, WIDTH, rotate90CW_5<T>(), myTypeName + " matrix rotate90CW_5" );
    test_rotate90CW( dataX, dataY, HEIGHT, WIDTH, rotate90CW_6<T>(), myTypeName + " matrix rotate90CW_6" );
    
    std::string temp5( myTypeName + " matrix rotate90Clockwise" );
    summarize( temp5.c_str(), (HEIGHT*WIDTH), iterations, kDontShowGMeans, kDontShowPenalty );
    
    
    test_rotate90CCW( dataX, dataY, HEIGHT, WIDTH, rotate90CCW_1<T>(), myTypeName + " matrix rotate90CW_1" );
    test_rotate90CCW( dataX, dataY, HEIGHT, WIDTH, rotate90CCW_2<T>(), myTypeName + " matrix rotate90CW_2" );
    test_rotate90CCW( dataX, dataY, HEIGHT, WIDTH, rotate90CCW_3<T>(), myTypeName + " matrix rotate90CW_3" );
    test_rotate90CCW( dataX, dataY, HEIGHT, WIDTH, rotate90CCW_4<T>(), myTypeName + " matrix rotate90CW_4" );
    test_rotate90CCW( dataX, dataY, HEIGHT, WIDTH, rotate90CCW_5<T>(), myTypeName + " matrix rotate90CW_5" );
    test_rotate90CCW( dataX, dataY, HEIGHT, WIDTH, rotate90CCW_6<T>(), myTypeName + " matrix rotate90CW_6" );
    
    std::string temp6( myTypeName + " matrix rotate90CounterClockwise" );
    summarize( temp6.c_str(), (HEIGHT*WIDTH), iterations, kDontShowGMeans, kDontShowPenalty );


    iterations = base_iterations;
    

    // Now the inplace versions

    iterations /= 3;
    test_flipH_inplace( dataX, dataY, HEIGHT, WIDTH, flipHorizontal_inplace1<T>(), myTypeName + " matrix flipHorizontal_inplace1" );
    test_flipH_inplace( dataX, dataY, HEIGHT, WIDTH, flipHorizontal_inplace2<T>(), myTypeName + " matrix flipHorizontal_inplace2" );
    test_flipH_inplace( dataX, dataY, HEIGHT, WIDTH, flipHorizontal_inplace3<T>(), myTypeName + " matrix flipHorizontal_inplace3" );
    test_flipH_inplace( dataX, dataY, HEIGHT, WIDTH, flipHorizontal_inplace4<T>(), myTypeName + " matrix flipHorizontal_inplace4" );
    test_flipH_inplace( dataX, dataY, HEIGHT, WIDTH, flipHorizontal_inplace5<T>(), myTypeName + " matrix flipHorizontal_inplace5" );
    test_flipH_inplace( dataX, dataY, HEIGHT, WIDTH, flipHorizontal_inplace6<T>(), myTypeName + " matrix flipHorizontal_inplace6" );
    test_flipH_inplace( dataX, dataY, HEIGHT, WIDTH, flipHorizontal_inplace7<T>(), myTypeName + " matrix flipHorizontal_inplace7" );
    test_flipH_inplace( dataX, dataY, HEIGHT, WIDTH, flipHorizontal_inplace8<T>(), myTypeName + " matrix flipHorizontal_inplace8" );

    std::string temp7( myTypeName + " matrix flipHorizontal inplace" );
    summarize( temp7.c_str(), (HEIGHT*WIDTH), iterations, kDontShowGMeans, kDontShowPenalty );


    iterations = base_iterations;
    test_flipV_inplace( dataX, dataY, HEIGHT, WIDTH, flipVertical_inplace1<T>(), myTypeName + " matrix flipVertical_inplace1" );
    test_flipV_inplace( dataX, dataY, HEIGHT, WIDTH, flipVertical_inplace2<T>(), myTypeName + " matrix flipVertical_inplace2" );
    test_flipV_inplace( dataX, dataY, HEIGHT, WIDTH, flipVertical_inplace3<T>(), myTypeName + " matrix flipVertical_inplace3" );
    test_flipV_inplace( dataX, dataY, HEIGHT, WIDTH, flipVertical_inplace4<T>(), myTypeName + " matrix flipVertical_inplace4" );
    test_flipV_inplace( dataX, dataY, HEIGHT, WIDTH, flipVertical_inplace5<T>(), myTypeName + " matrix flipVertical_inplace5" );
    test_flipV_inplace( dataX, dataY, HEIGHT, WIDTH, flipVertical_inplace6<T>(), myTypeName + " matrix flipVertical_inplace6" );
    test_flipV_inplace( dataX, dataY, HEIGHT, WIDTH, flipVertical_inplace7<T>(), myTypeName + " matrix flipVertical_inplace7" );
    test_flipV_inplace( dataX, dataY, HEIGHT, WIDTH, flipVertical_inplace8<T>(), myTypeName + " matrix flipVertical_inplace8" );

    std::string temp8( myTypeName + " matrix flipVertical inplace" );
    summarize( temp8.c_str(), (HEIGHT*WIDTH), iterations, kDontShowGMeans, kDontShowPenalty );



    iterations /= 3;
    test_flip180_inplace( dataX, dataY, HEIGHT, WIDTH, flip180_inplace1<T>(), myTypeName + " matrix flip180_inplace1" );
    test_flip180_inplace( dataX, dataY, HEIGHT, WIDTH, flip180_inplace2<T>(), myTypeName + " matrix flip180_inplace2" );
    test_flip180_inplace( dataX, dataY, HEIGHT, WIDTH, flip180_inplace3<T>(), myTypeName + " matrix flip180_inplace3" );
    test_flip180_inplace( dataX, dataY, HEIGHT, WIDTH, flip180_inplace4<T>(), myTypeName + " matrix flip180_inplace4" );
    test_flip180_inplace( dataX, dataY, HEIGHT, WIDTH, flip180_inplace5<T>(), myTypeName + " matrix flip180_inplace5" );
    
    std::string temp9( myTypeName + " matrix flip180 inplace" );
    summarize( temp9.c_str(), (HEIGHT*WIDTH), iterations, kDontShowGMeans, kDontShowPenalty );



    // Transpose and 90 degree rotates inplace only work if the matrix is square
    auto minSize = std::min( HEIGHT, WIDTH );
    
    test_transpose_inplace( dataX, dataY, minSize, minSize, transpose_inplace1<T>(), myTypeName + " matrix transpose_inplace1" );
    test_transpose_inplace( dataX, dataY, minSize, minSize, transpose_inplace2<T>(), myTypeName + " matrix transpose_inplace2" );
    test_transpose_inplace( dataX, dataY, minSize, minSize, transpose_inplace3<T>(), myTypeName + " matrix transpose_inplace3" );
    test_transpose_inplace( dataX, dataY, minSize, minSize, transpose_inplace4<T>(), myTypeName + " matrix transpose_inplace4" );
    test_transpose_inplace( dataX, dataY, minSize, minSize, transpose_inplace5<T>(), myTypeName + " matrix transpose_inplace5" );
    test_transpose_inplace( dataX, dataY, minSize, minSize, transpose_inplace6<T>(), myTypeName + " matrix transpose_inplace6" );
    test_transpose_inplace( dataX, dataY, minSize, minSize, transpose_inplace7<T>(), myTypeName + " matrix transpose_inplace7" );
    test_transpose_inplace( dataX, dataY, minSize, minSize, transpose_inplace8<T>(), myTypeName + " matrix transpose_inplace8" );
    
    std::string temp10( myTypeName + " matrix transpose inplace" );
    summarize( temp10.c_str(), (minSize*minSize), iterations, kDontShowGMeans, kDontShowPenalty );



    test_rotate90CW_inplace( dataX, dataY, minSize, minSize, rotate90CW_inplace1<T>(), myTypeName + " matrix rotate90CW_inplace1" );
    test_rotate90CW_inplace( dataX, dataY, minSize, minSize, rotate90CW_inplace2<T>(), myTypeName + " matrix rotate90CW_inplace2" );
    test_rotate90CW_inplace( dataX, dataY, minSize, minSize, rotate90CW_inplace3<T>(), myTypeName + " matrix rotate90CW_inplace3" );
    test_rotate90CW_inplace( dataX, dataY, minSize, minSize, rotate90CW_inplace4<T>(), myTypeName + " matrix rotate90CW_inplace4" );
    test_rotate90CW_inplace( dataX, dataY, minSize, minSize, rotate90CW_inplace5<T>(), myTypeName + " matrix rotate90CW_inplace5" );
    test_rotate90CW_inplace( dataX, dataY, minSize, minSize, rotate90CW_inplace6<T>(), myTypeName + " matrix rotate90CW_inplace6" );
    test_rotate90CW_inplace( dataX, dataY, minSize, minSize, rotate90CW_inplace7<T>(), myTypeName + " matrix rotate90CW_inplace7" );
    
    std::string temp11( myTypeName + " matrix rotate90Clockwise inplace" );
    summarize( temp11.c_str(), (minSize*minSize), iterations, kDontShowGMeans, kDontShowPenalty );
    
    
    test_rotate90CCW_inplace( dataX, dataY, minSize, minSize, rotate90CCW_inplace1<T>(), myTypeName + " matrix rotate90CCW_inplace1" );
    test_rotate90CCW_inplace( dataX, dataY, minSize, minSize, rotate90CCW_inplace2<T>(), myTypeName + " matrix rotate90CCW_inplace2" );
    test_rotate90CCW_inplace( dataX, dataY, minSize, minSize, rotate90CCW_inplace3<T>(), myTypeName + " matrix rotate90CCW_inplace3" );
    test_rotate90CCW_inplace( dataX, dataY, minSize, minSize, rotate90CCW_inplace4<T>(), myTypeName + " matrix rotate90CCW_inplace4" );
    test_rotate90CCW_inplace( dataX, dataY, minSize, minSize, rotate90CCW_inplace5<T>(), myTypeName + " matrix rotate90CCW_inplace5" );
    test_rotate90CCW_inplace( dataX, dataY, minSize, minSize, rotate90CCW_inplace6<T>(), myTypeName + " matrix rotate90CCW_inplace6" );
    
    std::string temp12( myTypeName + " matrix rotate90CounterClockwise inplace" );
    summarize( temp12.c_str(), (minSize*minSize), iterations, kDontShowGMeans, kDontShowPenalty );
    

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
    scrand( init_value );


    // Results depend on data size, not type, so we don't have to test every type.
    // But some compilers special case floating point differently for some reason - so we have to test that.
    TestOneType<uint8_t>();

    iterations /= 2;
    TestOneType<uint16_t>();
    
    iterations /= 2;
    TestOneType<uint32_t>();
    TestOneType<float>();
    
    //iterations /= 2;
    TestOneType<int64_t>();
    TestOneType<double>();


#if WORKS_BUT_UNNECESSARY
    TestOneType<int8_t>();
    TestOneType<int16_t>();
    TestOneType<int32_t>();
    TestOneType<uint64_t>();
    TestOneType<long double>();
#endif
    
    return 0;
}

// the end
/******************************************************************************/
/******************************************************************************/
