/*
    Copyright 2008 Adobe Systems Incorporated
    Copyright 2018 Chris Cox
    Distributed under the MIT License (see accompanying file LICENSE_1_0_0.txt
    or a copy at http://stlab.adobe.com/licenses.html )


Goal:  Test performance of various idioms for creating histograms.

Note:  Creating histograms is very common in numerical, graphics, audio,
        some compression, and some cryptographic applications.


Assumptions:
    1) The compiler will optimize histogram creation.

    2) The compiler should recognize ineffecient histogram idioms and substitute efficient methods.
        NOTE: Best methods depend greatly on the CPU architecture (cache, branching, vector, etc.).


*/

#include "benchmark_stdint.hpp"
#include <cstddef>
#include <cstdio>
#include <ctime>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include "benchmark_results.h"
#include "benchmark_timer.h"
#include "benchmark_algorithms.h"

/******************************************************************************/
/******************************************************************************/

// this constant may need to be adjusted to give reasonable minimum times
// For best results, times should be about 1.0 seconds for the minimum test run
int iterations = 45000;

// about 32k to 256k of data
// this is intended to remain within the L2 cache of most common CPUs
const int SIZE = 32000;


// initial value for filling our arrays, may be changed from the command line
int32_t init_value = 3;

/******************************************************************************/

// our global arrays of numbers

uint8_t inputData8[SIZE];
uint16_t inputData16[SIZE];
float inputData32[SIZE];
double inputData64[SIZE];

uint16_t histogram16[ 256 ];
uint32_t histogram32[ 256 ];
uint64_t histogram64[ 256 ];

uint16_t histogram16S[ 65536 ];
uint32_t histogram32S[ 65536 ];
uint64_t histogram64S[ 65536 ];

uint64_t referenceHistogram[ 256 ];
uint64_t referenceHistogramS[ 65536 ];

const int float_hist_size = 8192;
uint16_t histogram16F[ float_hist_size ];
uint32_t histogram32F[ float_hist_size ];
uint64_t histogram64F[ float_hist_size ];
uint64_t referenceHistogramF[ float_hist_size ];

/******************************************************************************/
/******************************************************************************/

template <typename Iterator, typename T>
void fill_random_float(Iterator first, Iterator last, const T minVal, const T maxVal) {
    T range = maxVal - minVal;
    T scale = range / ((1L << 24) - 1);
    while (first != last) {
        T value = static_cast<T>( ((rand() & 0x00FFFFFF) * scale) + minVal );
        *first++ = value;
    }
}

/******************************************************************************/
/******************************************************************************/

template<typename T, typename X>
inline void verify_histogram(X* histogram, const char *label) {
    uint64_t j;
    const uint64_t *refHist = referenceHistogram;
    const size_t bitSize = (8*sizeof(T));
    uint64_t maxCount = 1;

    if (bitSize >= 32) {
        refHist = referenceHistogramF;
        maxCount = float_hist_size;
    } else
        maxCount = (1L << bitSize);
    
    if (bitSize == 16)
        refHist = referenceHistogramS;
    
    for (j = 0; j < maxCount; ++j) {
        if (histogram[j] != X(refHist[j])) {
            //printf("test %s failed (index %llu has count %llu vs %llu)\n", label, j, (uint64_t)refHist[j], (uint64_t)histogram[j] );
            std::cout << "test " << label << " failed (index " << j <<" has count " << refHist[j] << " vs " << histogram[j] <<" )" << std::endl;
            break;
        }
    }
}

/******************************************************************************/
/******************************************************************************/

// baseline - a trivial loop
template <typename T, typename X>
void test_histogram1(const T* input, const int count, X* histogram, const char *label) {
    int i, j;

    start_timer();

    for(i = 0; i < iterations; ++i) {
        memset( (void *)histogram, 0,  (1L << (8*sizeof(T)))*sizeof(X) );
        for (j = 0; j < count; ++j) {
            histogram[ input[j] ] += 1;
        }
    }
    verify_histogram<T,X>(histogram,label);

    record_result( timer(), label );
}

/******************************************************************************/

// unroll 4X
template <typename T, typename X>
void test_histogram2(const T* input, const int count, X* histogram, const char *label) {
    int i, j;

    start_timer();

    for(i = 0; i < iterations; ++i) {
        memset( (void *)histogram, 0,  (1L << (8*sizeof(T)))*sizeof(X) );
        for (j = 0; j < (count - 3); j += 4) {
            T value0 = input[j + 0];
            T value1 = input[j + 1];
            T value2 = input[j + 2];
            T value3 = input[j + 3];
            
            histogram[ value0 ] += 1;
            histogram[ value1 ] += 1;
            histogram[ value2 ] += 1;
            histogram[ value3 ] += 1;
        }
        for (; j < count; ++j) {
            histogram[ input[j] ] += 1;
        }
    }
    verify_histogram<T,X>(histogram,label);

    record_result( timer(), label );
}

/******************************************************************************/

// unroll 4x
// read 32 bits at a time
// must be special cased for a specific data size
template <typename X>
void test_histogram3(const uint8_t* input, const int count, X* histogram, const char *label) {
    int i, j;

    start_timer();

    for(i = 0; i < iterations; ++i) {
        memset( (void *)histogram, 0,  (1L << (8*sizeof(uint8_t)))*sizeof(X) );
        
        j = 0;
        
        // align to 32 bit boundary
        for ( ; j < count && ((intptr_t(input) & 3) != 0); ++j) {
            histogram[ input[j] ] += 1;
        }
        
        for (j = 0; j < (count - 3); j += 4) {
            uint32_t longValue0 = *( (uint32_t *)(input + j + 0) );
            
            uint8_t value0 = (longValue0 >> 24) & 0xFF;
            uint8_t value1 = (longValue0 >> 16) & 0xFF;
            uint8_t value2 = (longValue0 >> 8) & 0xFF;
            uint8_t value3 = longValue0 & 0xFF;
            
            histogram[ value0 ] += 1;
            histogram[ value1 ] += 1;
            histogram[ value2 ] += 1;
            histogram[ value3 ] += 1;
        }
        for (; j < count; ++j) {
            histogram[ input[j] ] += 1;
        }
    }
    verify_histogram<uint8_t,X>(histogram,label);

    record_result( timer(), label );
}

/******************************************************************************/

// unroll 4X
// read 64 bits at a time
// must be special cased for a specific data size
template <typename X>
void test_histogram3(const uint16_t* input, const int count, X* histogram, const char *label) {
    int i, j;

    start_timer();

    for(i = 0; i < iterations; ++i) {
        memset( (void *)histogram, 0,  (1L << (8*sizeof(uint16_t)))*sizeof(X) );
        
        j = 0;
        
        // align to 64 bit boundary
        for ( ; j < count && ((intptr_t(input) & 7) != 0); ++j) {
            histogram[ input[j] ] += 1;
        }
        
        for (j = 0; j < (count - 3); j += 4) {
            uint64_t longValue0 = *( (uint64_t *)(input + j + 0) );
            
            uint16_t value0 = (longValue0 >> 48) & 0xFFFF;
            uint16_t value1 = (longValue0 >> 32) & 0xFFFF;
            uint16_t value2 = (longValue0 >> 16) & 0xFFFF;
            uint16_t value3 = longValue0 & 0xFFFF;
            
            histogram[ value0 ] += 1;
            histogram[ value1 ] += 1;
            histogram[ value2 ] += 1;
            histogram[ value3 ] += 1;
        }
        for (; j < count; ++j) {
            histogram[ input[j] ] += 1;
        }
    }
    verify_histogram<uint16_t,X>(histogram,label);

    record_result( timer(), label );
}

/******************************************************************************/

// unroll 4X
// use a second histogram and sum at the end
template <typename T, typename X>
void test_histogram4(const T* input, const int count, X* histogram, const char *label) {
    int i, j;
    const size_t maxIndex =  (1L << (8*sizeof(T)));
    X histogram1[ maxIndex ];

    start_timer();

    for(i = 0; i < iterations; ++i) {
    
        memset( (void *)histogram, 0, maxIndex*sizeof(X) );
        memset( (void *)histogram1, 0, maxIndex*sizeof(X) );
        
        for (j = 0; j < (count - 3); j += 4) {
            T value0 = input[j + 0];
            T value1 = input[j + 1];
            T value2 = input[j + 2];
            T value3 = input[j + 3];
            
            histogram[ value0 ] += 1;
            histogram1[ value1 ] += 1;
            histogram[ value2 ] += 1;
            histogram1[ value3 ] += 1;
        }
        for (; j < count; ++j) {
            histogram[ input[j] ] += 1;
        }
        for (j = 0; j < maxIndex; ++j) {
            histogram[ j ] += histogram1[ j ];
        }
    }
    verify_histogram<T,X>(histogram,label);

    record_result( timer(), label );
}

/******************************************************************************/

#if _MSC_VER
#define WORKAROUND    static
#else
#define WORKAROUND
#endif

// unroll 4X
// use 3 extra histograms and sum at the end
template <typename T, typename X>
void test_histogram5(const T* input, const int count, X* histogram, const char *label) {
    int i, j;
    const size_t maxIndex =  (1L << (8*sizeof(T)));
    WORKAROUND X histogram1[ maxIndex ];    // static because MSVC won't allocate that much on the stack!
    WORKAROUND X histogram2[ maxIndex ];
    WORKAROUND X histogram3[ maxIndex ];

    start_timer();

    for(i = 0; i < iterations; ++i) {
    
        memset( (void *)histogram, 0, maxIndex*sizeof(X) );
        memset( (void *)histogram1, 0, maxIndex*sizeof(X) );
        memset( (void *)histogram2, 0, maxIndex*sizeof(X) );
        memset( (void *)histogram3, 0, maxIndex*sizeof(X) );
        
        for (j = 0; j < (count - 3); j += 4) {
            T value0 = input[j + 0];
            T value1 = input[j + 1];
            T value2 = input[j + 2];
            T value3 = input[j + 3];
            
            histogram[ value0 ] += 1;
            histogram1[ value1 ] += 1;
            histogram2[ value2 ] += 1;
            histogram3[ value3 ] += 1;
        }
        for (; j < count; ++j) {
            histogram[ input[j] ] += 1;
        }
        for (j = 0; j < maxIndex; ++j) {
            histogram[ j ] += histogram1[ j ] + histogram2[ j ] + histogram3[ j ];
        }
    }
    verify_histogram<T,X>(histogram,label);

    record_result( timer(), label );
}

/******************************************************************************/

// unroll 16x
// read 32 bits at a time
// use 3 extra histograms and sum at the end
// must be special cased for a specific data size
template <typename X>
void test_histogram6(const uint8_t* input, const int count, X* histogram, const char *label) {
    int i, j;
    const size_t maxIndex =  (1L << (8*sizeof(uint8_t)));
    X histogram1[ maxIndex ];
    X histogram2[ maxIndex ];
    X histogram3[ maxIndex ];

    start_timer();

    for(i = 0; i < iterations; ++i) {
    
        memset( (void *)histogram, 0, maxIndex*sizeof(X) );
        memset( (void *)histogram1, 0, maxIndex*sizeof(X) );
        memset( (void *)histogram2, 0, maxIndex*sizeof(X) );
        memset( (void *)histogram3, 0, maxIndex*sizeof(X) );
        
        j = 0;
        
        // align to 32 bit boundary
        for ( ; j < count && ((intptr_t(input) & 3) != 0); ++j) {
            histogram[ input[j] ] += 1;
        }
        
        for ( ; j < (count - 15); j += 16) {
            uint32_t longValue0 = *( (uint32_t *)(input + j + 0) );
            uint32_t longValue1 = *( (uint32_t *)(input + j + 4) );
            uint32_t longValue2 = *( (uint32_t *)(input + j + 8) );
            uint32_t longValue3 = *( (uint32_t *)(input + j + 12) );
            
            uint8_t value0 = (longValue0 >> 24) & 0xFF;
            uint8_t value1 = (longValue0 >> 16) & 0xFF;
            uint8_t value2 = (longValue0 >> 8) & 0xFF;
            uint8_t value3 = longValue0 & 0xFF;
            
            histogram[ value0 ] += 1;
            histogram1[ value1 ] += 1;
            histogram2[ value2 ] += 1;
            histogram3[ value3 ] += 1;
            
            uint8_t value4 = (longValue1 >> 24) & 0xFF;
            uint8_t value5 = (longValue1 >> 16) & 0xFF;
            uint8_t value6 = (longValue1 >> 8) & 0xFF;
            uint8_t value7 = longValue1 & 0xFF;
            
            histogram[ value4 ] += 1;
            histogram1[ value5 ] += 1;
            histogram2[ value6 ] += 1;
            histogram3[ value7 ] += 1;
            
            uint8_t value8 = (longValue2 >> 24) & 0xFF;
            uint8_t value9 = (longValue2 >> 16) & 0xFF;
            uint8_t valueA = (longValue2 >> 8) & 0xFF;
            uint8_t valueB = longValue2 & 0xFF;
            
            histogram[ value8 ] += 1;
            histogram1[ value9 ] += 1;
            histogram2[ valueA ] += 1;
            histogram3[ valueB ] += 1;
            
            uint8_t valueC = (longValue3 >> 24) & 0xFF;
            uint8_t valueD = (longValue3 >> 16) & 0xFF;
            uint8_t valueE = (longValue3 >> 8) & 0xFF;
            uint8_t valueF = longValue3 & 0xFF;
            
            histogram[ valueC ] += 1;
            histogram1[ valueD ] += 1;
            histogram2[ valueE ] += 1;
            histogram3[ valueF ] += 1;
        }
        
        for (; j < (count - 3); j += 4) {
            uint32_t longValue0 = *( (uint32_t *)(input + j + 0) );
            
            uint8_t value0 = (longValue0 >> 24) & 0xFF;
            uint8_t value1 = (longValue0 >> 16) & 0xFF;
            uint8_t value2 = (longValue0 >> 8) & 0xFF;
            uint8_t value3 = longValue0 & 0xFF;
            
            histogram[ value0 ] += 1;
            histogram1[ value1 ] += 1;
            histogram2[ value2 ] += 1;
            histogram3[ value3 ] += 1;
        }
        
        for (; j < count; ++j) {
            histogram[ input[j] ] += 1;
        }
        for (j = 0; j < maxIndex; ++j) {
            histogram[ j ] += histogram1[ j ] + histogram2[ j ] + histogram3[ j ];
        }
    }
    verify_histogram<uint8_t>(histogram,label);

    record_result( timer(), label );
}

/******************************************************************************/

// unroll 16x
// read 64*4 bits at a time
// use 3 extra histograms and sum at the end
// must be special cased for a specific data size
template <typename X>
void test_histogram6(const uint16_t* input, const int count, X* histogram, const char *label) {
    int i, j;
    const size_t maxIndex =  (1L << (8*sizeof(uint16_t)));
    WORKAROUND X histogram1[ maxIndex ];    // static because MSVC won't allocate that much on the stack!
    WORKAROUND X histogram2[ maxIndex ];
    WORKAROUND X histogram3[ maxIndex ];

    start_timer();

    for(i = 0; i < iterations; ++i) {
    
        memset( (void *)histogram, 0, maxIndex*sizeof(X) );
        memset( (void *)histogram1, 0, maxIndex*sizeof(X) );
        memset( (void *)histogram2, 0, maxIndex*sizeof(X) );
        memset( (void *)histogram3, 0, maxIndex*sizeof(X) );
        
        j = 0;
        
        // align to 64 bit boundary
        for ( ; j < count && ((intptr_t(input) & 7) != 0); ++j) {
            histogram[ input[j] ] += 1;
        }
        
        for ( ; j < (count - 15); j += 16) {
            uint64_t longValue0 = *( (uint64_t *)(input + j + 0) );
            uint64_t longValue1 = *( (uint64_t *)(input + j + 4) );
            uint64_t longValue2 = *( (uint64_t *)(input + j + 8) );
            uint64_t longValue3 = *( (uint64_t *)(input + j + 12) );
            
            uint16_t value0 = (longValue0 >> 48) & 0xFFFF;
            uint16_t value1 = (longValue0 >> 32) & 0xFFFF;
            uint16_t value2 = (longValue0 >> 16) & 0xFFFF;
            uint16_t value3 = longValue0 & 0xFFFF;
            
            histogram[ value0 ] += 1;
            histogram1[ value1 ] += 1;
            histogram2[ value2 ] += 1;
            histogram3[ value3 ] += 1;
            
            uint16_t value4 = (longValue1 >> 48) & 0xFFFF;
            uint16_t value5 = (longValue1 >> 32) & 0xFFFF;
            uint16_t value6 = (longValue1 >> 16) & 0xFFFF;
            uint16_t value7 = longValue1 & 0xFFFF;
            
            histogram[ value4 ] += 1;
            histogram1[ value5 ] += 1;
            histogram2[ value6 ] += 1;
            histogram3[ value7 ] += 1;
            
            uint16_t value8 = (longValue2 >> 48) & 0xFFFF;
            uint16_t value9 = (longValue2 >> 32) & 0xFFFF;
            uint16_t valueA = (longValue2 >> 16) & 0xFFFF;
            uint16_t valueB = longValue2 & 0xFFFF;
            
            histogram[ value8 ] += 1;
            histogram1[ value9 ] += 1;
            histogram2[ valueA ] += 1;
            histogram3[ valueB ] += 1;
            
            uint16_t valueC = (longValue3 >> 48) & 0xFFFF;
            uint16_t valueD = (longValue3 >> 32) & 0xFFFF;
            uint16_t valueE = (longValue3 >> 16) & 0xFFFF;
            uint16_t valueF = longValue3 & 0xFFFF;
            
            histogram[ valueC ] += 1;
            histogram1[ valueD ] += 1;
            histogram2[ valueE ] += 1;
            histogram3[ valueF ] += 1;
        }
        
        for (; j < (count - 3); j += 4) {
            uint64_t longValue0 = *( (uint64_t *)(input + j + 0) );
            
            uint16_t value0 = (longValue0 >> 48) & 0xFFFF;
            uint16_t value1 = (longValue0 >> 32) & 0xFFFF;
            uint16_t value2 = (longValue0 >> 16) & 0xFFFF;
            uint16_t value3 = longValue0 & 0xFFFF;
            
            histogram[ value0 ] += 1;
            histogram1[ value1 ] += 1;
            histogram2[ value2 ] += 1;
            histogram3[ value3 ] += 1;
        }
        
        for (; j < count; ++j) {
            histogram[ input[j] ] += 1;
        }
        for (j = 0; j < maxIndex; ++j) {
            histogram[ j ] += histogram1[ j ] + histogram2[ j ] + histogram3[ j ];
        }
    }
    verify_histogram<uint16_t,X>(histogram,label);

    record_result( timer(), label );
}

/******************************************************************************/
/******************************************************************************/

// baseline - a trivial loop
template <typename T, typename X>
void simple_histogramFloat(const T* input, const int count, X* histogram, const T minVal, const T maxVal, int ITERATIONS) {
    int i, j;

    const T range = maxVal - minVal;
    const double scale = (float_hist_size-1) / range;

    for(i = 0; i < ITERATIONS; ++i) {
        memset( (void *)histogram, 0,  float_hist_size*sizeof(X) );
        for (j = 0; j < count; ++j) {
            T value = input[j];
            int index = int( scale * (value - minVal) );
            if (index >= 0 && index < float_hist_size)    // reject NaN and out of range values
                histogram[ index ] += 1;
        }
    }
}

/******************************************************************************/

// baseline - a trivial loop
template <typename T, typename X>
void test_histogramFloat1(const T* input, const int count, X* histogram, const T minVal, const T maxVal, const char *label) {

    start_timer();
    simple_histogramFloat(input,count,histogram,minVal,maxVal,iterations);
    verify_histogram<T,X>(histogram,label);

    record_result( timer(), label );
}

/******************************************************************************/

// unroll 4X
template <typename T, typename X>
void test_histogramFloat2(const T* input, const int count, X* histogram, const T minVal, const T maxVal, const char *label) {
    int i, j;

    start_timer();

    const T range = maxVal - minVal;
    const double scale = (float_hist_size-1) / range;

    for(i = 0; i < iterations; ++i) {
        memset( (void *)histogram, 0,  float_hist_size*sizeof(X) );
        for (j = 0; j < (count-3); j+=4) {
            T value0 = input[j+0];
            T value1 = input[j+1];
            T value2 = input[j+2];
            T value3 = input[j+3];
            
            int index0 = int( scale * (value0 - minVal) );
            int index1 = int( scale * (value1 - minVal) );
            int index2 = int( scale * (value2 - minVal) );
            int index3 = int( scale * (value3 - minVal) );
            
            if (index0 >= 0 && index0 < float_hist_size)    // reject NaN and out of range values
                histogram[ index0 ] += 1;
            if (index1 >= 0 && index1 < float_hist_size)    // reject NaN and out of range values
                histogram[ index1 ] += 1;
            if (index2 >= 0 && index2 < float_hist_size)    // reject NaN and out of range values
                histogram[ index2 ] += 1;
            if (index3 >= 0 && index3 < float_hist_size)    // reject NaN and out of range values
                histogram[ index3 ] += 1;
        }
        for (; j < count; ++j) {
            T value0 = input[j];
            int index0 = int( scale * (value0 - minVal) );
            if (index0 >= 0 && index0 < float_hist_size)    // reject NaN and out of range values
                histogram[ index0 ] += 1;
        }
    }
    verify_histogram<T,X>(histogram,label);

    record_result( timer(), label );
}

/******************************************************************************/

// unroll 4X, use a second histogram and sum histograms at the end
template <typename T, typename X>
void test_histogramFloat4(const T* input, const int count, X* histogram, const T minVal, const T maxVal, const char *label) {
    int i, j;
    X histogram1[ float_hist_size ];

    start_timer();

    const T range = maxVal - minVal;
    const double scale = (float_hist_size-1) / range;

    for(i = 0; i < iterations; ++i) {
    
        memset( (void *)histogram, 0,  float_hist_size*sizeof(X) );
        memset( (void *)histogram1, 0,  float_hist_size*sizeof(X) );
        
        for (j = 0; j < (count-3); j+=4) {
            T value0 = input[j+0];
            T value1 = input[j+1];
            T value2 = input[j+2];
            T value3 = input[j+3];
            
            // reject NaN and out of range values
            int index0 = int( scale * (value0 - minVal) );
            int index1 = int( scale * (value1 - minVal) );
            int index2 = int( scale * (value2 - minVal) );
            int index3 = int( scale * (value3 - minVal) );
            
            if (index0 >= 0 && index0 < float_hist_size)    // reject NaN and out of range values
                histogram[ index0 ] += 1;
            if (index1 >= 0 && index1 < float_hist_size)    // reject NaN and out of range values
                histogram1[ index1 ] += 1;
            if (index2 >= 0 && index2 < float_hist_size)    // reject NaN and out of range values
                histogram[ index2 ] += 1;
            if (index3 >= 0 && index3 < float_hist_size)    // reject NaN and out of range values
                histogram1[ index3 ] += 1;
        }
        for (; j < count; ++j) {
            T value0 = input[j];
            int index0 = int( scale * (value0 - minVal) );
            if (index0 >= 0 && index0 < float_hist_size)    // reject NaN and out of range values
                histogram[ index0 ] += 1;
        }
        for (j = 0; j < float_hist_size; ++j) {
            histogram[ j ] += histogram1[ j ];
        }
    }
    verify_histogram<T,X>(histogram,label);

    record_result( timer(), label );
}

/******************************************************************************/

// unroll 4X, use 3 extra histograms and sum histograms at the end
template <typename T, typename X>
void test_histogramFloat5(const T* input, const int count, X* histogram, const T minVal, const T maxVal, const char *label) {
    int i, j;
    X histogram1[ float_hist_size ];
    X histogram2[ float_hist_size ];
    X histogram3[ float_hist_size ];

    start_timer();

    const T range = maxVal - minVal;
    const double scale = (float_hist_size-1) / range;

    for(i = 0; i < iterations; ++i) {
    
        memset( (void *)histogram, 0,  float_hist_size*sizeof(X) );
        memset( (void *)histogram1, 0,  float_hist_size*sizeof(X) );
        memset( (void *)histogram2, 0,  float_hist_size*sizeof(X) );
        memset( (void *)histogram3, 0,  float_hist_size*sizeof(X) );
        
        for (j = 0; j < (count-3); j+=4) {
            T value0 = input[j+0];
            T value1 = input[j+1];
            T value2 = input[j+2];
            T value3 = input[j+3];
            
            int index0 = int( scale * (value0 - minVal) );
            int index1 = int( scale * (value1 - minVal) );
            int index2 = int( scale * (value2 - minVal) );
            int index3 = int( scale * (value3 - minVal) );
            
            if (index0 >= 0 && index0 < float_hist_size)    // reject NaN and out of range values
                histogram[ index0 ] += 1;
            if (index1 >= 0 && index1 < float_hist_size)    // reject NaN and out of range values
                histogram1[ index1 ] += 1;
            if (index2 >= 0 && index2 < float_hist_size)    // reject NaN and out of range values
                histogram2[ index2 ] += 1;
            if (index3 >= 0 && index3 < float_hist_size)    // reject NaN and out of range values
                histogram3[ index3 ] += 1;
        }
        for (; j < count; ++j) {
            T value0 = input[j];
            int index0 = int( scale * (value0 - minVal) );
            if (index0 >= 0 && index0 < float_hist_size)    // reject NaN and out of range values
                histogram[ index0 ] += 1;
        }
        for (j = 0; j < float_hist_size; ++j) {
            histogram[ j ] += histogram1[ j ] + histogram2[ j ] + histogram3[ j ];
        }
    }
    verify_histogram<T,X>(histogram,label);

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

    srand( init_value );
    fill_random( inputData8, inputData8+SIZE );
    fill_random( inputData16, inputData16+SIZE );
    fill_random_float( inputData32, inputData32+SIZE, -400.0f, 20000.0f );
    fill_random_float( inputData64, inputData64+SIZE, -400.0f, 20000.0f );
    
    // create our reference histogram for comparison
    memset( (void *)referenceHistogram, 0, 256*sizeof(uint64_t) );
    for (i = 0; i < SIZE; ++i) {
        referenceHistogram[ inputData8[i] ] += 1;
    }
    memset( (void *)referenceHistogramS, 0, 65536*sizeof(uint64_t) );
    for (i = 0; i < SIZE; ++i) {
        referenceHistogramS[ inputData16[i] ] += 1;
    }




// uint8_t
    test_histogram1( inputData8, SIZE, histogram16, "uint16_t histogram1 of uint8_t");
    test_histogram1( inputData8, SIZE, histogram32, "uint32_t histogram1 of uint8_t");
    test_histogram1( inputData8, SIZE, histogram64, "uint64_t histogram1 of uint8_t");
    
    test_histogram2( inputData8, SIZE, histogram16, "uint16_t histogram2 of uint8_t");
    test_histogram2( inputData8, SIZE, histogram32, "uint32_t histogram2 of uint8_t");
    test_histogram2( inputData8, SIZE, histogram64, "uint64_t histogram2 of uint8_t");
    
    test_histogram3( inputData8, SIZE, histogram16, "uint16_t histogram3 of uint8_t");
    test_histogram3( inputData8, SIZE, histogram32, "uint32_t histogram3 of uint8_t");
    test_histogram3( inputData8, SIZE, histogram64, "uint64_t histogram3 of uint8_t");

    test_histogram4( inputData8, SIZE, histogram16, "uint16_t histogram4 of uint8_t");
    test_histogram4( inputData8, SIZE, histogram32, "uint32_t histogram4 of uint8_t");
    test_histogram4( inputData8, SIZE, histogram64, "uint64_t histogram4 of uint8_t");

    test_histogram5( inputData8, SIZE, histogram16, "uint16_t histogram5 of uint8_t");
    test_histogram5( inputData8, SIZE, histogram32, "uint32_t histogram5 of uint8_t");
    test_histogram5( inputData8, SIZE, histogram64, "uint64_t histogram5 of uint8_t");

    test_histogram6( inputData8, SIZE, histogram16, "uint16_t histogram6 of uint8_t");
    test_histogram6( inputData8, SIZE, histogram32, "uint32_t histogram6 of uint8_t");
    test_histogram6( inputData8, SIZE, histogram64, "uint64_t histogram6 of uint8_t");
    
    summarize("uint8_t histogram", SIZE, iterations, kDontShowGMeans, kDontShowPenalty );



// uint16_t
    test_histogram1( inputData16, SIZE, histogram16S, "uint16_t histogram1 of uint16_t");
    test_histogram1( inputData16, SIZE, histogram32S, "uint32_t histogram1 of uint16_t");
    test_histogram1( inputData16, SIZE, histogram64S, "uint64_t histogram1 of uint16_t");
    
    test_histogram2( inputData16, SIZE, histogram16S, "uint16_t histogram2 of uint16_t");
    test_histogram2( inputData16, SIZE, histogram32S, "uint32_t histogram2 of uint16_t");
    test_histogram2( inputData16, SIZE, histogram64S, "uint64_t histogram2 of uint16_t");
    
    test_histogram3( inputData16, SIZE, histogram16S, "uint16_t histogram3 of uint16_t");
    test_histogram3( inputData16, SIZE, histogram32S, "uint32_t histogram3 of uint16_t");
    test_histogram3( inputData16, SIZE, histogram64S, "uint64_t histogram3 of uint16_t");

    test_histogram4( inputData16, SIZE, histogram16S, "uint16_t histogram4 of uint16_t");
    test_histogram4( inputData16, SIZE, histogram32S, "uint32_t histogram4 of uint16_t");
    test_histogram4( inputData16, SIZE, histogram64S, "uint64_t histogram4 of uint16_t");

    test_histogram5( inputData16, SIZE, histogram16S, "uint16_t histogram5 of uint16_t");
    test_histogram5( inputData16, SIZE, histogram32S, "uint32_t histogram5 of uint16_t");
    test_histogram5( inputData16, SIZE, histogram64S, "uint64_t histogram5 of uint16_t");

    test_histogram6( inputData16, SIZE, histogram16S, "uint16_t histogram6 of uint16_t");
    test_histogram6( inputData16, SIZE, histogram32S, "uint32_t histogram6 of uint16_t");
    test_histogram6( inputData16, SIZE, histogram64S, "uint64_t histogram6 of uint16_t");
    
    summarize("uint16_t histogram", SIZE, iterations, kDontShowGMeans, kDontShowPenalty );



// float
    iterations /= 4;    // keep times reasonable

    memset( (void *)referenceHistogramF, 0, float_hist_size*sizeof(uint64_t) );
    simple_histogramFloat( inputData32, SIZE, referenceHistogramF, -200.0f, 16000.0f, 1);

    test_histogramFloat1( inputData32, SIZE, histogram16F, -200.0f, 16000.0f, "uint16_t histogram1 of float");
    test_histogramFloat1( inputData32, SIZE, histogram32F, -200.0f, 16000.0f, "uint32_t histogram1 of float");
    test_histogramFloat1( inputData32, SIZE, histogram64F, -200.0f, 16000.0f, "uint64_t histogram1 of float");

    test_histogramFloat2( inputData32, SIZE, histogram16F, -200.0f, 16000.0f, "uint16_t histogram2 of float");
    test_histogramFloat2( inputData32, SIZE, histogram32F, -200.0f, 16000.0f, "uint32_t histogram2 of float");
    test_histogramFloat2( inputData32, SIZE, histogram64F, -200.0f, 16000.0f, "uint64_t histogram2 of float");

    test_histogramFloat4( inputData32, SIZE, histogram16F, -200.0f, 16000.0f, "uint16_t histogram4 of float");
    test_histogramFloat4( inputData32, SIZE, histogram32F, -200.0f, 16000.0f, "uint32_t histogram4 of float");
    test_histogramFloat4( inputData32, SIZE, histogram64F, -200.0f, 16000.0f, "uint64_t histogram4 of float");

    test_histogramFloat5( inputData32, SIZE, histogram16F, -200.0f, 16000.0f, "uint16_t histogram5 of float");
    test_histogramFloat5( inputData32, SIZE, histogram32F, -200.0f, 16000.0f, "uint32_t histogram5 of float");
    test_histogramFloat5( inputData32, SIZE, histogram64F, -200.0f, 16000.0f, "uint64_t histogram5 of float");

    summarize("float histogram", SIZE, iterations, kDontShowGMeans, kDontShowPenalty );



// double
    memset( (void *)referenceHistogramF, 0, float_hist_size*sizeof(uint64_t) );
    simple_histogramFloat( inputData64, SIZE, referenceHistogramF, -200.0, 16000.0, 1);

    test_histogramFloat1( inputData64, SIZE, histogram16F, -200.0, 16000.0, "uint16_t histogram1 of double");
    test_histogramFloat1( inputData64, SIZE, histogram32F, -200.0, 16000.0, "uint32_t histogram1 of double");
    test_histogramFloat1( inputData64, SIZE, histogram64F, -200.0, 16000.0, "uint64_t histogram1 of double");

    test_histogramFloat2( inputData64, SIZE, histogram16F, -200.0, 16000.0, "uint16_t histogram2 of double");
    test_histogramFloat2( inputData64, SIZE, histogram32F, -200.0, 16000.0, "uint32_t histogram2 of double");
    test_histogramFloat2( inputData64, SIZE, histogram64F, -200.0, 16000.0, "uint64_t histogram2 of double");

    test_histogramFloat4( inputData64, SIZE, histogram16F, -200.0, 16000.0, "uint16_t histogram4 of double");
    test_histogramFloat4( inputData64, SIZE, histogram32F, -200.0, 16000.0, "uint32_t histogram4 of double");
    test_histogramFloat4( inputData64, SIZE, histogram64F, -200.0, 16000.0, "uint64_t histogram4 of double");

    test_histogramFloat5( inputData64, SIZE, histogram16F, -200.0, 16000.0, "uint16_t histogram5 of double");
    test_histogramFloat5( inputData64, SIZE, histogram32F, -200.0, 16000.0, "uint32_t histogram5 of double");
    test_histogramFloat5( inputData64, SIZE, histogram64F, -200.0, 16000.0, "uint64_t histogram5 of double");
    
    summarize("double histogram", SIZE, iterations, kDontShowGMeans, kDontShowPenalty );


    return 0;
}

// the end
/******************************************************************************/
/******************************************************************************/
