/*
    Copyright 2018 Chris Cox
    Distributed under the MIT License (see accompanying file LICENSE_1_0_0.txt
    or a copy at http://stlab.adobe.com/licenses.html )


Goal:  Examine performance optimizations related to loop induction variables.


Assumptions:
    1) The compiler will normalize all loop types and optimize all equally.
        (this is a necessary step before doing induction variable analysis)
        
    2) The compiler will remove unused induction variables.
        This could happen due to several optimizations.

    2) The compiler will recognize induction variables with linear relations (x = a*b + c)
        and optimize out redundant variables.

    3) The compiler will apply strength reduction to induction variable usage.

    4) The compiler will remove bounds checks by recognizing or adjusting loop limits.
        (can be an explict loop optimization, or part of range propagation)


*/

#include "benchmark_stdint.hpp"
#include <cstddef>
#include <cstdio>
#include <ctime>
#include <cmath>
#include <cstdlib>
#include <algorithm>
#include <vector>
#include "benchmark_results.h"
#include "benchmark_timer.h"
#include "benchmark_algorithms.h"

/******************************************************************************/
/******************************************************************************/

// this constant may need to be adjusted to give reasonable minimum times
// For best results, times should be about 1.0 seconds for the minimum test run
int iterations = 600000;


// 8000 items, or about 32k of data
// this is intended to remain within the L2 cache of most common CPUs
const int SIZE = 8000;


// initial value for filling our arrays, may be changed from the command line
int init_value = 3;

/******************************************************************************/
/******************************************************************************/

template <typename T>
void copy_for_opt( const T* src, T* dst, const int count ) {
    
    int k;
    for ( k=0; k < count; ++k ) {
        dst[k] = src[k];
    }
}

/******************************************************************************/

// 3 induction variables, could be combined into one
template <typename T>
void copy_for1( const T* src, T* dst, const int count ) {
    
    int i, j, k;
    for ( i=0, j=0, k=0; k < count; ++i, ++j, ++k ) {
        dst[i] = src[j];
    }
}

/******************************************************************************/

// try simple scaling
template <typename T>
void copy_for2( const T* src, T* dst, const int count ) {
    
    int i, j, k;
    for ( i=0, j=0, k=0; k < count; i+=2, j+=2, ++k ) {
        dst[i/2] = src[j/2];
    }
}

/******************************************************************************/

// a scale that isn't just a shift
template <typename T>
void copy_for3( const T* src, T* dst, const int count ) {
    
    int i, j, k;
    for ( i=0, j=0, k=0; k < count; i+=3, j+=3, ++k ) {
        dst[i/3] = src[j/3];
    }
}

/******************************************************************************/

// try a larger, prime scale factor
template <typename T>
void copy_for4( const T* src, T* dst, const int count ) {
    
    int i, j, k;
    for ( i=0, j=0, k=0; k < count; i+=173, j+=173, ++k ) {
        dst[i/173] = src[j/173];
    }
}

/******************************************************************************/

// different scales
template <typename T>
void copy_for5( const T* src, T* dst, const int count ) {
    
    int i, j, k;
    for ( i=0, j=0, k=0; k < count; i+=3, j+=173, ++k ) {
        dst[i/3] = src[j/173];
    }
}

/******************************************************************************/

// use positive index offsets
template <typename T>
void copy_for6( const T* src, T* dst, const int count ) {
    
    int i, j, k;
    for ( i=2, j=99, k=0; k < count; ++i, ++j, ++k ) {
        dst[i-2] = src[j-99];
    }
}

/******************************************************************************/

// use a negative index offset
template <typename T>
void copy_for7( const T* src, T* dst, const int count ) {
    
    int i, j, k;
    for ( i=-255, j=99, k=0; k < count; ++i, ++j, ++k ) {
        dst[i+255] = src[j-99];
    }
}

/******************************************************************************/

// offset indices and scale
template <typename T>
void copy_for8( const T* src, T* dst, const int count ) {
    
    int i, j, k;
    for ( i=99, j=-255, k=0; k < count; i+=3, j+=173, ++k ) {
        dst[(i-99)/3] = src[(j+255)/173];
    }
}

/******************************************************************************/

// offset the pointers
template <typename T>
void copy_for9( const T* src, T* dst, const int count ) {
    
    dst -= 99;
    src += 255;
    int i, j, k;
    for ( i=99, j=-255, k=0; k < count; ++i, ++j, ++k ) {
        dst[i] = src[j];
    }
}

/******************************************************************************/

// offset the pointers, and scale
template <typename T>
void copy_for10( const T* src, T* dst, const int count ) {
    
    dst -= 99;
    src += 255;
    int i, j, k;
    for ( i=99*3, j=-255*173, k=0; k < count; i+=3, j+=173, ++k ) {
        dst[i/3] = src[j/173];
    }
}

/******************************************************************************/

// derived induction variables
template <typename T>
void copy_for11( const T* src, T* dst, const int count ) {
    
    int k;
    for ( k=0; k < count; ++k ) {
        int x = (k * 250) + (k * 5) + k;
        int y = (k * 500) + (k * 13) - k;
        dst[(x >> 8)] = src[(y >> 9)];
    }
}

/******************************************************************************/

// multiple derived induction variables
template <typename T>
void copy_for12( const T* src, T* dst, const int count ) {
    
    int i, j, k;
    for ( i=0, j=0, k=0; k < count; ++i, ++j, ++k ) {
        int x = (i * 250) + (i * 5) + i;
        int y = (j * 500) + (j * 13) - j;
        dst[(x >> 8)] = src[(y >> 9)];
    }
}

/******************************************************************************/

// multiple derived induction variables, with offsets
template <typename T>
void copy_for13( const T* src, T* dst, const int count ) {
    
    int i, j, k;
    for ( i=61, j=-17, k=0; k < count; ++i, ++j, ++k ) {
        int x = (i * 250) + (i * 5) + i;
        int y = (j * 500) + (j * 13) - j;
        dst[(x >> 8)-61] = src[(y >> 9)+17];
    }
}

/******************************************************************************/

// main induction variable also offset and scaled
template <typename T>
void copy_for14( const T* src, T* dst, const int count ) {
    
    int i, j, k;
    int end_count = count * 71;
    for ( i=99, j=-255, k=37; (k-37) < end_count; i+=3, j+=173, k+=71 ) {
        dst[(i-99)/3] = src[(j+255)/173];
    }
}

/******************************************************************************/

// double reversed
template <typename T>
void copy_for15( const T* src, T* dst, const int count ) {
    
    int i, j, k;
    for ( i=count, j=count, k=0; k < count; --i, --j, ++k ) {
        dst[count-i] = src[count-j];
    }
}

/******************************************************************************/

// double reversed, scaled
template <typename T>
void copy_for16( const T* src, T* dst, const int count ) {
    
    int i, j, k;
    for ( i=count*3, j=count*173, k=0; k < count; i-=3, j-=173, ++k ) {
        dst[count-(i/3)] = src[count-(j/173)];
    }
}

/******************************************************************************/

// double reversed, loop reverse
template <typename T>
void copy_for17( const T* src, T* dst, const int count ) {
    
    int i, j, k;
    for ( i=count, j=count, k=count; k > 0; --i, --j, --k ) {
        dst[count-i] = src[count-j];
    }
}

/******************************************************************************/

// double reversed, scaled, loop reverse
template <typename T>
void copy_for18( const T* src, T* dst, const int count ) {
    
    int i, j, k;
    for ( i=count*3, j=count*173, k=count; k > 0; i-=3, j-=173, --k ) {
        dst[count-(i/3)] = src[count-(j/173)];
    }
}

/******************************************************************************/
/******************************************************************************/

template <typename T>
void copy_while_opt( const T* src, T* dst, const int count ) {
    
    int k = 0;
    while ( k < count ) {
        dst[k] = src[k];
        ++k;
    }
}

/******************************************************************************/

template <typename T>
void copy_while1( const T* src, T* dst, const int count ) {
    
    int i=0, j=0, k=0;
    while ( k < count ) {
        dst[i] = src[j];
        ++i;
        ++j;
        ++k;
    }
}

/******************************************************************************/

template <typename T>
void copy_while2( const T* src, T* dst, const int count ) {
    
    int i=0, j=0, k=0;
    while ( k < count ) {
        dst[i/2] = src[j/2];
        i+=2;
        j+=2;
        ++k;
    }
}

/******************************************************************************/

template <typename T>
void copy_while3( const T* src, T* dst, const int count ) {
    
    int i=0, j=0, k=0;
    while ( k < count ) {
        dst[i/3] = src[j/3];
        i+=3;
        j+=3;
        ++k;
    }
}

/******************************************************************************/

template <typename T>
void copy_while4( const T* src, T* dst, const int count ) {
    
    int i=0, j=0, k=0;
    while ( k < count ) {
        dst[i/173] = src[j/173];
        i+=173;
        j+=173;
        ++k;
    }
}

/******************************************************************************/

template <typename T>
void copy_while5( const T* src, T* dst, const int count ) {
    
    int i=0, j=0, k=0;
    while ( k < count ) {
        dst[i/3] = src[j/173];
        i+=3;
        j+=173;
        ++k;
    }
}

/******************************************************************************/

// use positive index offsets
template <typename T>
void copy_while6( const T* src, T* dst, const int count ) {
    
    int i=2, j=99, k=0;
    while ( k < count ) {
        dst[i-2] = src[j-99];
        ++i;
        ++j;
        ++k;
    }
}

/******************************************************************************/

// use a negative index offset
template <typename T>
void copy_while7( const T* src, T* dst, const int count ) {
    
    int i=-255, j=99, k=0;
    while ( k < count) {
        dst[i+255] = src[j-99];
        ++i;
        ++j;
        ++k;
    }
}

/******************************************************************************/

// offset indices and scale
template <typename T>
void copy_while8( const T* src, T* dst, const int count ) {
    
    int i=99, j=-255, k=0;
    while ( k < count ) {
        dst[(i-99)/3] = src[(j+255)/173];
        i+=3;
        j+=173;
        ++k;
    }
}

/******************************************************************************/

// offset the pointers
template <typename T>
void copy_while9( const T* src, T* dst, const int count ) {
    
    int i=99, j=-255, k=0;
    dst -= 99;
    src += 255;
    while ( k < count) {
        dst[i] = src[j];
        ++i;
        ++j;
        ++k;
    }
}

/******************************************************************************/

// offset the pointers, and scale
template <typename T>
void copy_while10( const T* src, T* dst, const int count ) {
    
    int i=99*3, j=-255*173, k=0;
    dst -= 99;
    src += 255;
    while ( k < count ) {
        dst[i/3] = src[j/173];
        i+=3;
        j+=173;
        ++k;
    }
}

/******************************************************************************/

template <typename T>
void copy_while11( const T* src, T* dst, const int count ) {
    
    int k=0;
    while ( k < count ) {
        int x = (k * 250) + (k * 5) + k;
        int y = (k * 500) + (k * 13) - k;
        dst[(x >> 8)] = src[(y >> 9)];
        ++k;
    }
}

/******************************************************************************/

template <typename T>
void copy_while12( const T* src, T* dst, const int count ) {
    
    int i=0, j=0, k=0;
    while ( k < count ) {
        int x = (i * 250) + (i * 5) + i;
        int y = (j * 500) + (j * 13) - j;
        dst[(x >> 8)] = src[(y >> 9)];
        ++i;
        ++j;
        ++k;
    }
}

/******************************************************************************/

// multiple derived induction variables, with offsets
template <typename T>
void copy_while13( const T* src, T* dst, const int count ) {
    
    int i=61, j=-17, k=0;
    while ( k < count ) {
        int x = (i * 250) + (i * 5) + i;
        int y = (j * 500) + (j * 13) - j;
        dst[(x >> 8)-61] = src[(y >> 9)+17];
        ++i;
        ++j;
        ++k;
    }
}

/******************************************************************************/

// main induction variable also offset and scaled
template <typename T>
void copy_while14( const T* src, T* dst, const int count ) {
    
    int i=99, j=-255, k=37;
    int end_count = count * 71;
    while ( (k-37) < end_count ) {
        dst[(i-99)/3] = src[(j+255)/173];
        i+=3;
        j+=173;
        k+=71;
    }
}

/******************************************************************************/

// double reversed
template <typename T>
void copy_while15( const T* src, T* dst, const int count ) {
    
    int i=count, j=count, k=0;
    while ( k < count ) {
        dst[count-i] = src[count-j];
        --i;
        --j;
        ++k;
    }
}

/******************************************************************************/

// double reversed, scaled
template <typename T>
void copy_while16( const T* src, T* dst, const int count ) {
    
    int i=count*3, j=count*173, k=0;
    while ( k < count) {
        dst[count-i/3] = src[count-j/173];
        i-=3;
        j-=173;
        ++k;
    }
}

/******************************************************************************/

// double reversed, loop reverse
template <typename T>
void copy_while17( const T* src, T* dst, const int count ) {
    
    int i=count, j=count, k=count;
    while ( k > 0 ) {
        dst[count-i] = src[count-j];
        --i;
        --j;
        --k;
    }
}

/******************************************************************************/

// double reversed, scaled, loop reverse
template <typename T>
void copy_while18( const T* src, T* dst, const int count ) {
    
    int i=count*3, j=count*173, k=count;
    while ( k > 0 ) {
        dst[count-i/3] = src[count-j/173];
        i-=3;
        j-=173;
        --k;
    }
}

/******************************************************************************/
/******************************************************************************/

template <typename T>
void copy_do_opt( const T* src, T* dst, const int count ) {
    
    int k = 0;
    if (count > 0)
    do {
        dst[k] = src[k];
        ++k;
    } while (k < count );
}

/******************************************************************************/

template <typename T>
void copy_do1( const T* src, T* dst, const int count ) {
    
    int i=0, j=0, k=0;
    if (count > 0)
    do {
        dst[i] = src[j];
        ++i;
        ++j;
        ++k;
    } while (k < count );
}

/******************************************************************************/

template <typename T>
void copy_do2( const T* src, T* dst, const int count ) {
    
    int i=0, j=0, k=0;
    if (count > 0)
    do {
        dst[i/2] = src[j/2];
        i+=2;
        j+=2;
        ++k;
    } while (k < count );
}

/******************************************************************************/

template <typename T>
void copy_do3( const T* src, T* dst, const int count ) {
    
    int i=0, j=0, k=0;
    if (count > 0)
    do {
        dst[i/3] = src[j/3];
        i+=3;
        j+=3;
        ++k;
    } while (k < count );
}

/******************************************************************************/

template <typename T>
void copy_do4( const T* src, T* dst, const int count ) {
    
    int i=0, j=0, k=0;
    if (count > 0)
    do {
        dst[i/173] = src[j/173];
        i+=173;
        j+=173;
        ++k;
    } while (k < count );
}

/******************************************************************************/

template <typename T>
void copy_do5( const T* src, T* dst, const int count ) {
    
    int i=0, j=0, k=0;
    if (count > 0)
    do {
        dst[i/3] = src[j/173];
        i+=3;
        j+=173;
        ++k;
    } while (k < count );
}

/******************************************************************************/

// use positive index offsets
template <typename T>
void copy_do6( const T* src, T* dst, const int count ) {
    
    int i=2, j=99, k=0;
    if (count > 0)
    do {
        dst[i-2] = src[j-99];
        ++i;
        ++j;
        ++k;
    } while (k < count );
}

/******************************************************************************/

// use a negative index offset
template <typename T>
void copy_do7( const T* src, T* dst, const int count ) {
    
    int i=-255, j=99, k=0;
    if (count > 0)
    do {
        dst[i+255] = src[j-99];
        ++i;
        ++j;
        ++k;
    } while (k < count );
}

/******************************************************************************/

// offset indices and scale
template <typename T>
void copy_do8( const T* src, T* dst, const int count ) {
    
    int i=99, j=-255, k=0;
    if (count > 0)
    do {
        dst[(i-99)/3] = src[(j+255)/173];
        i+=3;
        j+=173;
        ++k;
    } while (k < count );
}

/******************************************************************************/

// offset the pointers
template <typename T>
void copy_do9( const T* src, T* dst, const int count ) {
    
    int i=99, j=-255, k=0;
    dst -= 99;
    src += 255;
    if (count > 0)
    do {
        dst[i] = src[j];
        ++i;
        ++j;
        ++k;
    } while (k < count );
}

/******************************************************************************/

// offset the pointers, and scale
template <typename T>
void copy_do10( const T* src, T* dst, const int count ) {
    
    int i=99*3, j=-255*173, k=0;
    dst -= 99;
    src += 255;
    if (count > 0)
    do {
        dst[i/3] = src[j/173];
        i+=3;
        j+=173;
        ++k;
    } while (k < count );
}

/******************************************************************************/

template <typename T>
void copy_do11( const T* src, T* dst, const int count ) {
    
    int k=0;
    if (count > 0)
    do {
        int x = (k * 250) + (k * 5) + k;
        int y = (k * 500) + (k * 13) - k;
        dst[(x >> 8)] = src[(y >> 9)];
        ++k;
    } while (k < count );
}

/******************************************************************************/

template <typename T>
void copy_do12( const T* src, T* dst, const int count ) {
    
    int i=0, j=0, k=0;
    if (count > 0)
    do {
        int x = (i * 250) + (i * 5) + i;
        int y = (j * 500) + (j * 13) - j;
        dst[(x >> 8)] = src[(y >> 9)];
        ++i;
        ++j;
        ++k;
    } while (k < count );
}

/******************************************************************************/

// multiple derived induction variables, with offsets
template <typename T>
void copy_do13( const T* src, T* dst, const int count ) {
    
    int i=61, j=-17, k=0;
    if (count > 0)
    do {
        int x = (i * 250) + (i * 5) + i;
        int y = (j * 500) + (j * 13) - j;
        dst[(x >> 8)-61] = src[(y >> 9)+17];
        ++i;
        ++j;
        ++k;
    } while (k < count );
}

/******************************************************************************/

// main induction variable also offset and scaled
template <typename T>
void copy_do14( const T* src, T* dst, const int count ) {
    
    int i=99, j=-255, k=37;
    int end_count = count * 71;
    if (end_count > 0)
    do {
        dst[(i-99)/3] = src[(j+255)/173];
        i+=3;
        j+=173;
        k+=71;
    } while ( (k-37) < end_count );
}

/******************************************************************************/

// double reversed
template <typename T>
void copy_do15( const T* src, T* dst, const int count ) {
    
    int i=count, j=count, k=0;
    if (count > 0)
    do {
        dst[count-i] = src[count-j];
        --i;
        --j;
        ++k;
    } while ( k < count );
}

/******************************************************************************/

// double reversed, scaled
template <typename T>
void copy_do16( const T* src, T* dst, const int count ) {
    
    int i=count*3, j=count*173, k=0;
    if (count > 0)
    do {
        dst[count-i/3] = src[count-j/173];
        i-=3;
        j-=173;
        ++k;
    } while ( k < count);
}

/******************************************************************************/

// double reversed, loop reverse
template <typename T>
void copy_do17( const T* src, T* dst, const int count ) {
    
    int i=count, j=count, k=count;
    if (count > 0)
    do {
        dst[count-i] = src[count-j];
        --i;
        --j;
        --k;
    } while ( k > 0 );
}

/******************************************************************************/

// double reversed, scaled, loop reverse
template <typename T>
void copy_do18( const T* src, T* dst, const int count ) {
    
    int i=count*3, j=count*173, k=count;
    if (count > 0)
    do {
        dst[count-i/3] = src[count-j/173];
        i-=3;
        j-=173;
        --k;
    } while ( k > 0 );
}

/******************************************************************************/
/******************************************************************************/

template <typename T>
void copy_goto_opt( const T* src, T* dst, const int count ) {
    
    int k = 0;
    if (count > 0)
    {
loop_start:
        dst[k] = src[k];
        ++k;
        if (k < count )
            goto loop_start;
    }
}

/******************************************************************************/

template <typename T>
void copy_goto1( const T* src, T* dst, const int count ) {
    
    int i=0, j=0, k=0;
    if (count > 0)
    {
loop_start:
        dst[i] = src[j];
        ++i;
        ++j;
        ++k;
        if (k < count )
            goto loop_start;
    }
}

/******************************************************************************/

template <typename T>
void copy_goto2( const T* src, T* dst, const int count ) {
    
    int i=0, j=0, k=0;
    if (count > 0)
    {
loop_start:
        dst[i/2] = src[j/2];
        i+=2;
        j+=2;
        ++k;
        if (k < count )
            goto loop_start;
    }
}

/******************************************************************************/

template <typename T>
void copy_goto3( const T* src, T* dst, const int count ) {
    
    int i=0, j=0, k=0;
    if (count > 0)
    {
loop_start:
        dst[i/3] = src[j/3];
        i+=3;
        j+=3;
        ++k;
        if (k < count )
            goto loop_start;
    }
}

/******************************************************************************/

template <typename T>
void copy_goto4( const T* src, T* dst, const int count ) {
    
    int i=0, j=0, k=0;
    if (count > 0)
    {
loop_start:
        dst[i/173] = src[j/173];
        i+=173;
        j+=173;
        ++k;
        if (k < count )
            goto loop_start;
    }
}

/******************************************************************************/

template <typename T>
void copy_goto5( const T* src, T* dst, const int count ) {
    
    int i=0, j=0, k=0;
    if (count > 0)
    {
loop_start:
        dst[i/3] = src[j/173];
        i+=3;
        j+=173;
        ++k;
        if (k < count )
            goto loop_start;
    }
}

/******************************************************************************/

// use positive index offsets
template <typename T>
void copy_goto6( const T* src, T* dst, const int count ) {
    
    int i=2, j=99, k=0;
    if (count > 0)
    {
loop_start:
        dst[i-2] = src[j-99];
        ++i;
        ++j;
        ++k;
        if (k < count )
            goto loop_start;
    }
}

/******************************************************************************/

// use a negative index offset
template <typename T>
void copy_goto7( const T* src, T* dst, const int count ) {
    
    int i=-255, j=99, k=0;
    if (count > 0)
    {
loop_start:
        dst[i+255] = src[j-99];
        ++i;
        ++j;
        ++k;
        if (k < count )
            goto loop_start;
    }
}

/******************************************************************************/

// offset indices and scale
template <typename T>
void copy_goto8( const T* src, T* dst, const int count ) {
    
    int i=99, j=-255, k=0;
    if (count > 0)
    {
loop_start:
        dst[(i-99)/3] = src[(j+255)/173];
        i+=3;
        j+=173;
        ++k;
        if (k < count )
            goto loop_start;
    }
}

/******************************************************************************/

// offset the pointers
template <typename T>
void copy_goto9( const T* src, T* dst, const int count ) {
    
    int i=99, j=-255, k=0;
    dst -= 99;
    src += 255;
    if (count > 0)
    {
loop_start:
        dst[i] = src[j];
        ++i;
        ++j;
        ++k;
        if (k < count )
            goto loop_start;
    }
}

/******************************************************************************/

// offset the pointers, and scale
template <typename T>
void copy_goto10( const T* src, T* dst, const int count ) {
    
    int i=99*3, j=-255*173, k=0;
    dst -= 99;
    src += 255;
    if (count > 0)
    {
loop_start:
        dst[i/3] = src[j/173];
        i+=3;
        j+=173;
        ++k;
        if (k < count )
            goto loop_start;
    }
}

/******************************************************************************/

template <typename T>
void copy_goto11( const T* src, T* dst, const int count ) {
    
    int k=0;
    if (count > 0)
    {
loop_start:
        int x = (k * 250) + (k * 5) + k;
        int y = (k * 500) + (k * 13) - k;
        dst[(x >> 8)] = src[(y >> 9)];
        ++k;
        if (k < count )
            goto loop_start;
    }
}

/******************************************************************************/

template <typename T>
void copy_goto12( const T* src, T* dst, const int count ) {
    
    int i=0, j=0, k=0;
    if (count > 0)
    {
loop_start:
        int x = (i * 250) + (i * 5) + i;
        int y = (j * 500) + (j * 13) - j;
        dst[(x >> 8)] = src[(y >> 9)];
        ++i;
        ++j;
        ++k;
        if (k < count )
            goto loop_start;
    }
}

/******************************************************************************/

// multiple derived induction variables, with offsets
template <typename T>
void copy_goto13( const T* src, T* dst, const int count ) {
    
    int i=61, j=-17, k=0;
    if (count > 0)
    {
loop_start:
        int x = (i * 250) + (i * 5) + i;
        int y = (j * 500) + (j * 13) - j;
        dst[(x >> 8)-61] = src[(y >> 9)+17];
        ++i;
        ++j;
        ++k;
        if (k < count )
            goto loop_start;
    }
}

/******************************************************************************/

// main induction variable also offset and scaled
template <typename T>
void copy_goto14( const T* src, T* dst, const int count ) {
    
    int i=99, j=-255, k=37;
    int end_count = count * 71;
    if (end_count > 0)
    {
loop_start:
        dst[(i-99)/3] = src[(j+255)/173];
        i+=3;
        j+=173;
        k+=71;
        if( (k-37) < end_count )
            goto loop_start;
    }
}

/******************************************************************************/

// double reversed
template <typename T>
void copy_goto15( const T* src, T* dst, const int count ) {
    
    int i=count, j=count, k=0;
    if (count > 0)
    {
loop_start:
        dst[count-i] = src[count-j];
        --i;
        --j;
        ++k;
        if ( k < count )
            goto loop_start;
    }
}

/******************************************************************************/

// double reversed, scaled
template <typename T>
void copy_goto16( const T* src, T* dst, const int count ) {
    
    int i=count*3, j=count*173, k=0;
    if (count > 0)
    {
loop_start:
        dst[count-i/3] = src[count-j/173];
        i-=3;
        j-=173;
        ++k;
        if ( k < count )
            goto loop_start;
    }
}

/******************************************************************************/

// double reversed, loop reverse
template <typename T>
void copy_goto17( const T* src, T* dst, const int count ) {
    
    int i=count, j=count, k=count;
    if (count > 0)
    {
loop_start:
        dst[count-i] = src[count-j];
        --i;
        --j;
        --k;
        if ( k > 0)
            goto loop_start;
    }
}

/******************************************************************************/

// double reversed, scaled, loop reverse
template <typename T>
void copy_goto18( const T* src, T* dst, const int count ) {
    
    int i=count*3, j=count*173, k=count;
    if (count > 0)
    {
loop_start:
        dst[count-i/3] = src[count-j/173];
        i-=3;
        j-=173;
        --k;
        if ( k > 0)
            goto loop_start;
    }
}

/******************************************************************************/
/******************************************************************************/

// range copy optimized by adjusting loop limits
template <typename T>
void copyrange_for_opt( const T* src, T* dst, const int begin, const int end, const int /* count */ ) {
    
    int k;
    for ( k=begin; k < end; ++k ) {
        dst[k] = src[k];
    }
}

/******************************************************************************/

// explicit range checks with adjusted limits
template <typename T>
void copyrange_for1( const T* src, T* dst, const int begin, const int end, const int count ) {
    
    int k;
    for ( k=begin; k < end; ++k ) {
        if (k < begin)
            continue;
        if (k >= end)
            continue;
        dst[k] = src[k];
    }
}

/******************************************************************************/

// explicit range checks, unadjusted limits
template <typename T>
void copyrange_for2( const T* src, T* dst, const int begin, const int end, const int count ) {
    
    int k;
    for ( k=0; k < count; ++k ) {
        if (k < begin)
            continue;
        if (k >= end)
            continue;
        dst[k] = src[k];
    }
}

/******************************************************************************/

// add extra induction variables
template <typename T>
void copyrange_for3( const T* src, T* dst, const int begin, const int end, const int count ) {
    
    int i, j, k;
    for ( i=0, j=0, k=0; k < count; ++i, ++j, ++k ) {
        if (i < begin)
            continue;
        if (i >= end)
            continue;
        if (j < begin)
            continue;
        if (j >= end)
            continue;
        dst[i] = src[j];
    }
}

/******************************************************************************/

// reverse loop
template <typename T>
void copyrange_for4( const T* src, T* dst, const int begin, const int end, const int count ) {
    
    int i, j, k;
    for ( i=0, j=0, k=count; k > 0; ++i, ++j, --k ) {
        if (i < begin)
            continue;
        if (i >= end)
            continue;
        if (j < begin)
            continue;
        if (j >= end)
            continue;
        dst[i] = src[j];
    }
}

/******************************************************************************/

// scaling of added induction variables
template <typename T>
void copyrange_for5( const T* src, T* dst, const int begin, const int end, const int count ) {
    
    const int begin7 = begin*7;
    const int end7 = end*7;
    const int begin173 = begin*173;
    const int end173 = end*173;
    int i, j, k;
    for ( i=0, j=0, k=0; k < count; i+=7, j+=173, ++k ) {
        if (i < begin7)
            continue;
        if (i >= end7)
            continue;
        if (j < begin173)
            continue;
        if (j >= end173)
            continue;
        dst[i/7] = src[j/173];
    }
}

/******************************************************************************/

// explicit range checks with adjusted limits, reverse loop
template <typename T>
void copyrange_for6( const T* src, T* dst, const int begin, const int end, const int count ) {
    
    int k;
    for ( k=end-1; k >= begin; --k ) {
        if (k < begin)
            continue;
        if (k >= end)
            continue;
        dst[k] = src[k];
    }
}

/******************************************************************************/

// explicit range checks, unadjusted limits, reverse loop
template <typename T>
void copyrange_for7( const T* src, T* dst, const int begin, const int end, const int count ) {
    
    int k;
    for ( k=count; k > 0; --k ) {
        if (k < begin)
            continue;
        if (k >= end)
            continue;
        dst[k] = src[k];
    }
}

/******************************************************************************/

// reverse loop, scaled
template <typename T>
void copyrange_for8( const T* src, T* dst, const int begin, const int end, const int count ) {
    
    int k;
    const int begin7 = begin*7;
    const int end7 = end*7;
    const int count7 = count*7;
    for ( k=count7; k > 0; k-=7 ) {
        if (k < begin7)
            continue;
        if (k >= end7)
            continue;
        dst[k/7] = src[k/7];
    }
}

/******************************************************************************/
/******************************************************************************/

// unused induction ints
template <typename T>
void copy_for_unused1( const T* src, T* dst, const int count ) {
    
    int i, j, k, l, m, q, r, s, x, y, z;
    for ( i=0, j=0, k=0, l=0, m=0, q=0, r=0, s=0, x=0, y=0, z=0;
        k < count;
        ++i, ++j, ++k, ++l, ++m, ++q, ++r, ++s, ++x, ++y, ++z ) {
        dst[i] = src[j];
    }
}

/******************************************************************************/

// unused induction doubles
template <typename T>
void copy_for_unused2( const T* src, T* dst, const int count ) {
    
    int i, j, k;
    double l, m, q, r, s, x, y, z;
    for ( i=0, j=0, k=0, l=0, m=0, q=0, r=0, s=0, x=0, y=0, z=0;
        k < count;
        ++i, ++j, ++k, ++l, ++m, ++q, ++r, ++s, ++x, ++y, ++z ) {
        dst[i] = src[j];
    }
}

/******************************************************************************/

// unused induction ints, scaled
template <typename T>
void copy_for_unused3( const T* src, T* dst, const int count ) {
    
    int i, j, k, l, m, q, r, s, x, y, z;
    for ( i=0, j=0, k=0, l=0, m=0, q=0, r=0, s=0, x=0, y=0, z=0;
        k < count;
        ++i, ++j, ++k, l+=3, m+=7, q+=173, r+=99, s+=83, x+=42, y+=13, z+=257 ) {
        dst[i] = src[j];
    }
}

/******************************************************************************/

// unused induction doubles, scaled
template <typename T>
void copy_for_unused4( const T* src, T* dst, const int count ) {
    
    int i, j, k;
    double l, m, q, r, s, x, y, z;
    for ( i=0, j=0, k=0, l=0, m=0, q=0, r=0, s=0, x=0, y=0, z=0;
        k < count;
        ++i, ++j, ++k, l+=3.0, m+=7.1, q+=173.2, r+=99.3, s+=83.4, x+=42.5, y+=13.6, z+=257.7 ) {
        dst[i] = src[j];
    }
}

/******************************************************************************/

// unused induction ints, scaled, derived values
template <typename T>
void copy_for_unused5( const T* src, T* dst, const int count ) {
    
    int i, j, k, l, m, q, r, s, x, y, z;
    for ( i=0, j=0, k=0, l=0, m=0, q=0, r=0, s=0, x=0, y=0, z=0;
        k < count;
        ++i, ++j, ++k, l+=3, m+=7, q+=173, r+=99, s+=83, x+=42, y+=13, z+=257 ) {
        int mm = m / 7;
        int ll = l / 3;
        int qq = (q / 9) ^ 0xfeedfaceL;
        int rr = (r / 11) & ~0x0f;
        int ss = s / 3;
        int xx = (x / 6) + 5;
        int yy = (y / 11) + 6;
        int zz = (z / 15) + 7;
        dst[i] = src[j];
    }
}

/******************************************************************************/

// unused induction doubles, scaled, derived values
template <typename T>
void copy_for_unused6( const T* src, T* dst, const int count ) {
    
    int i, j, k;
    double l, m, q, r, s, x, y, z;
    for ( i=0, j=0, k=0, l=0, m=0, q=0, r=0, s=0, x=0, y=0, z=0;
        k < count;
        ++i, ++j, ++k, l+=3.0, m+=7.1, q+=173.2, r+=99.3, s+=83.4, x+=42.5, y+=13.6, z+=257.7 ) {
        double mm = m / 7.0;
        double ll = l / 3.1;
        double qq = q / 9.2;
        double rr = r / 11.3;
        double ss = s / 3.4;
        double xx = (x / 6.5) + 5.3;
        double yy = (y / 11.6) + 6.2;
        double zz = (z / 15.7) + 7.1;
        dst[i] = src[j];
    }
}

/******************************************************************************/

// unused induction ints
template <typename T>
void copy_for_unused7( const T* src, T* dst, const int count ) {
    
    int i, j, k, l, m, q, r, s, x, y, z;
    for ( i=0, j=0, k=0, l=0, m=0, q=0, r=0, s=0, x=count, y=count, z=count;
        k < count;
        ++i, ++j, ++k, ++l, ++m, ++q, ++r, ++s, --x, --y, --z ) {
        dst[i] = src[j];
    }
}

/******************************************************************************/
/******************************************************************************/

// linear math derived variables
template <typename T>
void copy_for_reduce1( const T* src, T* dst, const int count ) {
    
    int k;
    for ( k=0; k < count; ++k ) {
        int ii = ((k * 7) + k) / 2;
        int jj = ((k * 17) - k) / 2;
        dst[(ii>>2)] = src[(jj>>3)];
    }
}

/******************************************************************************/

// linear math derived variables
template <typename T>
void copy_for_reduce2( const T* src, T* dst, const int count ) {
    
    int k;
    for ( k=0; k < count; ++k ) {
        int ii = ((k * 7) + k) / 2;
        int jj = ((k * 15) + k) / 2;
        dst[(ii>>2)] = src[(jj>>3)];
    }
}

/******************************************************************************/

// linear math derived variables
template <typename T>
void copy_for_reduce3( const T* src, T* dst, const int count ) {
    
    int k;
    for ( k=0; k < count; ++k ) {
        int ii = (k * 8) / 2;
        int jj = (k * 16) / 2;
        dst[(ii>>2)] = src[(jj>>3)];
    }
}

/******************************************************************************/

// linear math derived variables
template <typename T>
void copy_for_reduce4( const T* src, T* dst, const int count ) {
    
    int k;
    for ( k=0; k < count; ++k ) {
        int ii = k * 4;
        int jj = k * 8;
        dst[(ii>>2)] = src[(jj>>3)];
    }
}

/******************************************************************************/

// linear math derived variables
template <typename T>
void copy_for_reduce5( const T* src, T* dst, const int count ) {
    
    int k;
    for ( k=0; k < count; ++k ) {
        int ii = (k+1) * 4;
        int jj = (k+3) * 8;
        dst[(ii>>2)-1] = src[(jj>>3)-3];
    }
}

/******************************************************************************/

// linear math derived variables
template <typename T>
void copy_for_reduce6( const T* src, T* dst, const int count ) {
    
    int k;
    for ( k=0; k < count; ++k ) {
        int ii = (k+1) * 4;
        int jj = (k+3) * 8;
        dst[((ii-4)>>2)] = src[((jj-24)>>3)];
    }
}

/******************************************************************************/

// linear math derived variables, getting complex
template <typename T>
void copy_for_reduce7( const T* src, T* dst, const int count ) {
    
    int k;
    for ( k=0; k < count; ++k ) {
        int ii = (k+2) * 4 + 8;
        int jj = (k+5) * 8 + 16;
        dst[((ii-4)>>2)-3] = src[((jj-24)>>3)-4];
    }
}

/******************************************************************************/

// linear math derived variables, insanity edition
// Yes, it is still the same simple copy loop.
template <typename T>
void copy_for_reduce8( const T* src, T* dst, const int count ) {
    
    int k;
    for ( k=0; k < count; ++k ) {
        int ii = (k+2) * 3 + k + 10;
        int jj = (k+5) * 9 - k + 11;
        dst[((ii-4)>>2)-3] = src[((jj-24)>>3)-4];
    }
}

/******************************************************************************/
/******************************************************************************/

template <typename T, typename Move >
void test_copy(const T *source, T *dest, int count, Move copier, const char *label) {
    int i;
    
    fill_random<T*,T>( dest, dest+count );

    start_timer();

    for(i = 0; i < iterations; ++i) {
        copier( source, dest, count );
    }
    
    record_result( timer(), label );
    
    if ( memcmp(dest, source, count*sizeof(T)) != 0 )
        printf("test %s failed\n", label);
}

/******************************************************************************/

template <typename T, typename Move >
void test_copyrange(const T *source, T *dest, int start, int stop, int count, Move copier, const char *label) {
    int i;
    
    copy(source,source+count,dest);
    fill_random<T*,T>( dest+start, dest+stop );

    start_timer();

    for(i = 0; i < iterations; ++i) {
        copier( source, dest, start, stop, count );
    }
    
    record_result( timer(), label );
    
    if ( memcmp(dest, source, count*sizeof(T)) != 0 )
        printf("test %s failed\n", label);
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
    if (argc > 2) init_value = (int) atoi(argv[2]);
    
    int32_t intSrc[ SIZE ];
    int32_t intDst[ SIZE ];
    
    
    srand( init_value );
    fill_random<int32_t*,int32_t>( intSrc, intSrc+SIZE );



    test_copy( &intSrc[0], &intDst[0], SIZE, copy_for_opt<int32_t>, "int32_t for induction copy opt" );
    test_copy( &intSrc[0], &intDst[0], SIZE, copy_for1<int32_t>, "int32_t for induction copy1" );
    test_copy( &intSrc[0], &intDst[0], SIZE, copy_for2<int32_t>, "int32_t for induction copy2" );
    test_copy( &intSrc[0], &intDst[0], SIZE, copy_for3<int32_t>, "int32_t for induction copy3" );
    test_copy( &intSrc[0], &intDst[0], SIZE, copy_for4<int32_t>, "int32_t for induction copy4" );
    test_copy( &intSrc[0], &intDst[0], SIZE, copy_for5<int32_t>, "int32_t for induction copy5" );
    test_copy( &intSrc[0], &intDst[0], SIZE, copy_for6<int32_t>, "int32_t for induction copy6" );
    test_copy( &intSrc[0], &intDst[0], SIZE, copy_for7<int32_t>, "int32_t for induction copy7" );
    test_copy( &intSrc[0], &intDst[0], SIZE, copy_for8<int32_t>, "int32_t for induction copy8" );
    test_copy( &intSrc[0], &intDst[0], SIZE, copy_for9<int32_t>, "int32_t for induction copy9" );
    test_copy( &intSrc[0], &intDst[0], SIZE, copy_for10<int32_t>, "int32_t for induction copy10" );
    test_copy( &intSrc[0], &intDst[0], SIZE, copy_for11<int32_t>, "int32_t for induction copy11" );
    test_copy( &intSrc[0], &intDst[0], SIZE, copy_for12<int32_t>, "int32_t for induction copy12" );
    test_copy( &intSrc[0], &intDst[0], SIZE, copy_for13<int32_t>, "int32_t for induction copy13" );
    test_copy( &intSrc[0], &intDst[0], SIZE, copy_for14<int32_t>, "int32_t for induction copy14" );
    test_copy( &intSrc[0], &intDst[0], SIZE, copy_for15<int32_t>, "int32_t for induction copy15" );
    test_copy( &intSrc[0], &intDst[0], SIZE, copy_for16<int32_t>, "int32_t for induction copy16" );
    test_copy( &intSrc[0], &intDst[0], SIZE, copy_for17<int32_t>, "int32_t for induction copy17" );
    test_copy( &intSrc[0], &intDst[0], SIZE, copy_for18<int32_t>, "int32_t for induction copy18" );
    
    summarize("for loop induction copy", SIZE, iterations, kDontShowGMeans, kDontShowPenalty );
    

    test_copy( &intSrc[0], &intDst[0], SIZE, copy_while_opt<int32_t>, "int32_t while induction copy opt" );
    test_copy( &intSrc[0], &intDst[0], SIZE, copy_while1<int32_t>, "int32_t while induction copy1" );
    test_copy( &intSrc[0], &intDst[0], SIZE, copy_while2<int32_t>, "int32_t while induction copy2" );
    test_copy( &intSrc[0], &intDst[0], SIZE, copy_while3<int32_t>, "int32_t while induction copy3" );
    test_copy( &intSrc[0], &intDst[0], SIZE, copy_while4<int32_t>, "int32_t while induction copy4" );
    test_copy( &intSrc[0], &intDst[0], SIZE, copy_while5<int32_t>, "int32_t while induction copy5" );
    test_copy( &intSrc[0], &intDst[0], SIZE, copy_while6<int32_t>, "int32_t while induction copy6" );
    test_copy( &intSrc[0], &intDst[0], SIZE, copy_while7<int32_t>, "int32_t while induction copy7" );
    test_copy( &intSrc[0], &intDst[0], SIZE, copy_while8<int32_t>, "int32_t while induction copy8" );
    test_copy( &intSrc[0], &intDst[0], SIZE, copy_while9<int32_t>, "int32_t while induction copy9" );
    test_copy( &intSrc[0], &intDst[0], SIZE, copy_while10<int32_t>, "int32_t while induction copy10" );
    test_copy( &intSrc[0], &intDst[0], SIZE, copy_while11<int32_t>, "int32_t while induction copy11" );
    test_copy( &intSrc[0], &intDst[0], SIZE, copy_while12<int32_t>, "int32_t while induction copy12" );
    test_copy( &intSrc[0], &intDst[0], SIZE, copy_while13<int32_t>, "int32_t while induction copy13" );
    test_copy( &intSrc[0], &intDst[0], SIZE, copy_while14<int32_t>, "int32_t while induction copy14" );
    test_copy( &intSrc[0], &intDst[0], SIZE, copy_while15<int32_t>, "int32_t while induction copy15" );
    test_copy( &intSrc[0], &intDst[0], SIZE, copy_while16<int32_t>, "int32_t while induction copy16" );
    test_copy( &intSrc[0], &intDst[0], SIZE, copy_while17<int32_t>, "int32_t while induction copy17" );
    test_copy( &intSrc[0], &intDst[0], SIZE, copy_while18<int32_t>, "int32_t while induction copy18" );
    
    summarize("while loop induction copy", SIZE, iterations, kDontShowGMeans, kDontShowPenalty );
    

    test_copy( &intSrc[0], &intDst[0], SIZE, copy_do_opt<int32_t>, "int32_t do induction copy opt" );
    test_copy( &intSrc[0], &intDst[0], SIZE, copy_do1<int32_t>, "int32_t do induction copy1" );
    test_copy( &intSrc[0], &intDst[0], SIZE, copy_do2<int32_t>, "int32_t do induction copy2" );
    test_copy( &intSrc[0], &intDst[0], SIZE, copy_do3<int32_t>, "int32_t do induction copy3" );
    test_copy( &intSrc[0], &intDst[0], SIZE, copy_do4<int32_t>, "int32_t do induction copy4" );
    test_copy( &intSrc[0], &intDst[0], SIZE, copy_do5<int32_t>, "int32_t do induction copy5" );
    test_copy( &intSrc[0], &intDst[0], SIZE, copy_do6<int32_t>, "int32_t do induction copy6" );
    test_copy( &intSrc[0], &intDst[0], SIZE, copy_do7<int32_t>, "int32_t do induction copy7" );
    test_copy( &intSrc[0], &intDst[0], SIZE, copy_do8<int32_t>, "int32_t do induction copy8" );
    test_copy( &intSrc[0], &intDst[0], SIZE, copy_do9<int32_t>, "int32_t do induction copy9" );
    test_copy( &intSrc[0], &intDst[0], SIZE, copy_do10<int32_t>, "int32_t do induction copy10" );
    test_copy( &intSrc[0], &intDst[0], SIZE, copy_do11<int32_t>, "int32_t do induction copy11" );
    test_copy( &intSrc[0], &intDst[0], SIZE, copy_do12<int32_t>, "int32_t do induction copy12" );
    test_copy( &intSrc[0], &intDst[0], SIZE, copy_do13<int32_t>, "int32_t do induction copy13" );
    test_copy( &intSrc[0], &intDst[0], SIZE, copy_do14<int32_t>, "int32_t do induction copy14" );
    test_copy( &intSrc[0], &intDst[0], SIZE, copy_do15<int32_t>, "int32_t do induction copy15" );
    test_copy( &intSrc[0], &intDst[0], SIZE, copy_do16<int32_t>, "int32_t do induction copy16" );
    test_copy( &intSrc[0], &intDst[0], SIZE, copy_do17<int32_t>, "int32_t do induction copy17" );
    test_copy( &intSrc[0], &intDst[0], SIZE, copy_do18<int32_t>, "int32_t do induction copy18" );
    
    summarize("do loop induction copy", SIZE, iterations, kDontShowGMeans, kDontShowPenalty );
    

    test_copy( &intSrc[0], &intDst[0], SIZE, copy_goto_opt<int32_t>, "int32_t goto induction copy opt" );
    test_copy( &intSrc[0], &intDst[0], SIZE, copy_goto1<int32_t>, "int32_t goto induction copy1" );
    test_copy( &intSrc[0], &intDst[0], SIZE, copy_goto2<int32_t>, "int32_t goto induction copy2" );
    test_copy( &intSrc[0], &intDst[0], SIZE, copy_goto3<int32_t>, "int32_t goto induction copy3" );
    test_copy( &intSrc[0], &intDst[0], SIZE, copy_goto4<int32_t>, "int32_t goto induction copy4" );
    test_copy( &intSrc[0], &intDst[0], SIZE, copy_goto5<int32_t>, "int32_t goto induction copy5" );
    test_copy( &intSrc[0], &intDst[0], SIZE, copy_goto6<int32_t>, "int32_t goto induction copy6" );
    test_copy( &intSrc[0], &intDst[0], SIZE, copy_goto7<int32_t>, "int32_t goto induction copy7" );
    test_copy( &intSrc[0], &intDst[0], SIZE, copy_goto8<int32_t>, "int32_t goto induction copy8" );
    test_copy( &intSrc[0], &intDst[0], SIZE, copy_goto9<int32_t>, "int32_t goto induction copy9" );
    test_copy( &intSrc[0], &intDst[0], SIZE, copy_goto10<int32_t>, "int32_t goto induction copy10" );
    test_copy( &intSrc[0], &intDst[0], SIZE, copy_goto11<int32_t>, "int32_t goto induction copy11" );
    test_copy( &intSrc[0], &intDst[0], SIZE, copy_goto12<int32_t>, "int32_t goto induction copy12" );
    test_copy( &intSrc[0], &intDst[0], SIZE, copy_goto13<int32_t>, "int32_t goto induction copy13" );
    test_copy( &intSrc[0], &intDst[0], SIZE, copy_goto14<int32_t>, "int32_t goto induction copy14" );
    test_copy( &intSrc[0], &intDst[0], SIZE, copy_goto15<int32_t>, "int32_t goto induction copy15" );
    test_copy( &intSrc[0], &intDst[0], SIZE, copy_goto16<int32_t>, "int32_t goto induction copy16" );
    test_copy( &intSrc[0], &intDst[0], SIZE, copy_goto17<int32_t>, "int32_t goto induction copy17" );
    test_copy( &intSrc[0], &intDst[0], SIZE, copy_goto18<int32_t>, "int32_t goto induction copy18" );
    
    summarize("goto loop induction copy", SIZE, iterations, kDontShowGMeans, kDontShowPenalty );



    test_copyrange( &intSrc[0], &intDst[0], 29, SIZE-31, SIZE, copyrange_for_opt<int32_t>, "int32_t for induction copyrange opt" );
    test_copyrange( &intSrc[0], &intDst[0], 29, SIZE-31, SIZE, copyrange_for1<int32_t>, "int32_t for induction copyrange1" );
    test_copyrange( &intSrc[0], &intDst[0], 29, SIZE-31, SIZE, copyrange_for2<int32_t>, "int32_t for induction copyrange2" );
    test_copyrange( &intSrc[0], &intDst[0], 29, SIZE-31, SIZE, copyrange_for3<int32_t>, "int32_t for induction copyrange3" );
    test_copyrange( &intSrc[0], &intDst[0], 29, SIZE-31, SIZE, copyrange_for4<int32_t>, "int32_t for induction copyrange4" );
    test_copyrange( &intSrc[0], &intDst[0], 29, SIZE-31, SIZE, copyrange_for5<int32_t>, "int32_t for induction copyrange5" );
    test_copyrange( &intSrc[0], &intDst[0], 29, SIZE-31, SIZE, copyrange_for6<int32_t>, "int32_t for induction copyrange6" );
    test_copyrange( &intSrc[0], &intDst[0], 29, SIZE-31, SIZE, copyrange_for7<int32_t>, "int32_t for induction copyrange7" );
    test_copyrange( &intSrc[0], &intDst[0], 29, SIZE-31, SIZE, copyrange_for8<int32_t>, "int32_t for induction copyrange8" );
    
    summarize("for loop induction copyrange", SIZE, iterations, kDontShowGMeans, kDontShowPenalty );



    test_copy( &intSrc[0], &intDst[0], SIZE, copy_for_opt<int32_t>, "int32_t for induction copy opt verify1" );
    test_copy( &intSrc[0], &intDst[0], SIZE, copy_for_unused1<int32_t>, "int32_t for induction copy unused1" );
    test_copy( &intSrc[0], &intDst[0], SIZE, copy_for_unused2<int32_t>, "int32_t for induction copy unused2" );
    test_copy( &intSrc[0], &intDst[0], SIZE, copy_for_unused3<int32_t>, "int32_t for induction copy unused3" );
    test_copy( &intSrc[0], &intDst[0], SIZE, copy_for_unused4<int32_t>, "int32_t for induction copy unused4" );
    test_copy( &intSrc[0], &intDst[0], SIZE, copy_for_unused5<int32_t>, "int32_t for induction copy unused5" );
    test_copy( &intSrc[0], &intDst[0], SIZE, copy_for_unused6<int32_t>, "int32_t for induction copy unused6" );
    test_copy( &intSrc[0], &intDst[0], SIZE, copy_for_unused7<int32_t>, "int32_t for induction copy unused7" );
    
    summarize("for loop unused induction copy", SIZE, iterations, kDontShowGMeans, kDontShowPenalty );



    test_copy( &intSrc[0], &intDst[0], SIZE, copy_for_opt<int32_t>, "int32_t for induction copy opt verify2" );
    test_copy( &intSrc[0], &intDst[0], SIZE, copy_for_reduce1<int32_t>, "int32_t for induction copy reduce1" );
    test_copy( &intSrc[0], &intDst[0], SIZE, copy_for_reduce2<int32_t>, "int32_t for induction copy reduce2" );
    test_copy( &intSrc[0], &intDst[0], SIZE, copy_for_reduce3<int32_t>, "int32_t for induction copy reduce3" );
    test_copy( &intSrc[0], &intDst[0], SIZE, copy_for_reduce4<int32_t>, "int32_t for induction copy reduce4" );
    test_copy( &intSrc[0], &intDst[0], SIZE, copy_for_reduce5<int32_t>, "int32_t for induction copy reduce5" );
    test_copy( &intSrc[0], &intDst[0], SIZE, copy_for_reduce6<int32_t>, "int32_t for induction copy reduce6" );
    test_copy( &intSrc[0], &intDst[0], SIZE, copy_for_reduce7<int32_t>, "int32_t for induction copy reduce7" );
    test_copy( &intSrc[0], &intDst[0], SIZE, copy_for_reduce8<int32_t>, "int32_t for induction copy reduce8" );
    
    summarize("for loop reduce induction copy", SIZE, iterations, kDontShowGMeans, kDontShowPenalty );



    return 0;
}

// the end
/******************************************************************************/
/******************************************************************************/
