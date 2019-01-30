/*
    Copyright 2008-2009 Adobe Systems Incorporated
    Copyright 2018 Chris Cox
    Distributed under the MIT License (see accompanying file LICENSE_1_0_0.txt
    or a copy at http://stlab.adobe.com/licenses.html )


Goal:  Test compiler optimizations related to memset and hand coded memset loops.


Assumptions:

    1) The compiler will recognize memset like loops and optimize appropriately.
        This could be subtitution of calls to memset,
         or it could be just optimizing the loop to get the best throughput.
        On modern systems, cache hinting is usually required for best throughput:
            cachline zero, or cacheline set without reading.

    2) The library function memset should be optimized for small, medium, and large buffers.
        ie: low overhead for smaller buffer, highly hinted for large buffers.

    3) The STL function std::fill should be optimized for small, medium, and large buffers.
        ie: low overhead for smaller buffers, highly hinted for large buffers.



NOTE - On some OSes, memset calls into the VM system to set pages to constant values,
        thus running faster than the DRAM bandwidth would allow on large arrays.
    However, on those OSes, calling memset can hit mutexes and slow down
        significantly when called from threads.



*/

#include "benchmark_stdint.hpp"
#include <cstddef>
#include <cstdio>
#include <ctime>
#include <cstdlib>
#include <algorithm>
#include <string>
#include <deque>
#include "benchmark_results.h"
#include "benchmark_timer.h"
#include "benchmark_algorithms.h"
#include "benchmark_typenames.h"

/******************************************************************************/

// this constant may need to be adjusted to give reasonable minimum times
// For best results, times should be about 1.0 seconds for the minimum test run
int iterations = 400;


// 64 Megabytes, intended to be larger than L2 cache on common CPUs
// needs to be divisible by 8
const int SIZE = 64*1024*1024L;


// initial value for filling our arrays, may be changed from the command line
uint8_t init_value = 3;

/******************************************************************************/

static std::deque<std::string> gLabels;

/******************************************************************************/

inline void check_sum(size_t result, int count, const std::string &label) {
    size_t temp = size_t(count) * init_value;
    if (result != temp)
        printf("test %s failed\n", label.c_str() );
}

/******************************************************************************/
/******************************************************************************/

template <typename T >
void test_library_memset(T *first, int count, const uint8_t value, const std::string label) {
    int i;
    int bytes = count * sizeof(T);

    start_timer();

    for(i = 0; i < iterations; ++i) {
        memset( first, value, bytes );
    }

    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
    
    size_t zero = 0;
    uint8_t *first_byte = (uint8_t *)first;
    uint8_t *last_byte = first_byte + bytes;
    size_t temp = accumulate(first_byte, last_byte, zero);
    check_sum( temp, bytes, label );
}

/******************************************************************************/

template <typename T >
void test_std_fill(T *first, int count, const uint8_t start_value, const std::string label) {
    int i;
    
    T value = T(start_value);
    for (i=0;i<(sizeof(T)-1);++i)
        value = (value << 8) + T(start_value);

    start_timer();

    for(i = 0; i < iterations; ++i) {
        std::fill( first, first+count, value );
    }

    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
    
    size_t zero = 0;
    int bytes = count * sizeof(T);
    uint8_t *first_byte = (uint8_t *)first;
    uint8_t *last_byte = first_byte + bytes;
    check_sum( accumulate(first_byte, last_byte, zero), bytes, label );
}

/******************************************************************************/

template <typename T >
void test_iterator_fill(T *first, int count, const uint8_t start_value, const std::string label) {
    int i;
    T *last = first + count;
    
    T value = T(start_value);
    for (i=0;i<(sizeof(T)-1);++i)
        value = (value << 8) + T(start_value);
    
    start_timer();

    for(i = 0; i < iterations; ++i) {
        T *iter = first;
        
        while (iter != last)
            *iter++ = value;
    }

    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
    
    size_t zero = 0;
    int bytes = count * sizeof(T);
    uint8_t *first_byte = (uint8_t *)first;
    uint8_t *last_byte = first_byte + bytes;
    check_sum( accumulate(first_byte, last_byte, zero), bytes, label );
}

/******************************************************************************/

template <typename T >
void test_forloop_fill(T *first, int count, const uint8_t start_value, const std::string label) {
    int i;
    
    T value = T(start_value);
    for (i=0;i<(sizeof(T)-1);++i)
        value = (value << 8) + T(start_value);
    
    start_timer();

    for(i = 0; i < iterations; ++i) {
        int x;
        
        for (x = 0; x < count; ++x) {
            first[x] = value;
        }
    }

    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
    
    size_t zero = 0;
    int bytes = count * sizeof(T);
    uint8_t *first_byte = (uint8_t *)first;
    uint8_t *last_byte = first_byte + bytes;
    check_sum( accumulate(first_byte, last_byte, zero), bytes, label );
}

/******************************************************************************/

template <typename T >
void test_forloop_fill_unrolled(T *first, int count, const uint8_t start_value, const std::string label) {
    int i;
    
    T value = T(start_value);
    for (i=0;i<(sizeof(T)-1);++i)
        value = (value << 8) + T(start_value);
    
    start_timer();
    
    for(i = 0; i < iterations; ++i) {
        int x;
        
        for (x = 0; x < (count - 7); x += 8) {
            first[x+0] = value;
            first[x+1] = value;
            first[x+2] = value;
            first[x+3] = value;
            first[x+4] = value;
            first[x+5] = value;
            first[x+6] = value;
            first[x+7] = value;
        }
        
        // fill remaining bytes with simple byte loop
        for ( ; x < count; ++x) {
            first[x] = value;
        }
    }

    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
    
    size_t zero = 0;
    int bytes = count * sizeof(T);
    uint8_t *first_byte = (uint8_t *)first;
    uint8_t *last_byte = first_byte + bytes;
    check_sum( accumulate(first_byte, last_byte, zero), bytes, label );
}

/******************************************************************************/

// this really only works for bytes, but could be generalized for 16 and 32 bit values
template <typename T >
void test_forloop_fill_32(T *first, int count, const uint8_t start_value, const std::string label) {
    int i;
    int bytes = count * sizeof(T);
    uint8_t *first_byte = (uint8_t *)first;
    uint8_t *last_byte = first_byte + bytes;
    
    start_timer();
    
    for(i = 0; i < iterations; ++i) {
        int x = 0;
        
        if (bytes > 128) {
        
            uint32_t longValue = (uint32_t(start_value) << 8) | start_value;
            longValue |= (longValue << 16);
            
            // align to 4 byte boundary
            for (; x < bytes && (((intptr_t)first_byte+x) & 0x03); ++x) {
                first_byte[x] = uint8_t(start_value);
            }
            
            // fill as many 32 bit words as possible
            for ( ; x < (bytes - 15); x += 16) {
                *((uint32_t *)(first_byte + x + 0)) = longValue;
                *((uint32_t *)(first_byte + x + 4)) = longValue;
                *((uint32_t *)(first_byte + x + 8)) = longValue;
                *((uint32_t *)(first_byte + x + 12)) = longValue;
            }
            
            // fill leftover 4 byte words
            for ( ; x < (bytes - 3); x += 4) {
                *((uint32_t *)(first_byte + x + 0)) = longValue;
            }
        }
        
        // fill remaining bytes with simple byte loop
        for ( ; x < bytes; ++x) {
            first_byte[x] = start_value;
        }
    }

    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
    
    size_t zero = 0;
    check_sum( accumulate(first_byte, last_byte, zero), bytes, label );
}

/******************************************************************************/

// this really only works for bytes, but could be generalized for 16 and 32 bit values
template <typename T >
void test_forloop_fill_32cacheline(T *first, int count, const uint8_t start_value, const std::string label) {
    int i;
    int bytes = count * sizeof(T);
    uint8_t *first_byte = (uint8_t *)first;
    uint8_t *last_byte = first_byte + bytes;
    
    start_timer();
    
    for(i = 0; i < iterations; ++i) {
        int x = 0;
        
        if (bytes > 128) {
        
            uint32_t longValue = (uint32_t(start_value) << 8) | start_value;
            longValue |= (longValue << 16);
            
            // align to 4 byte boundary
            for (; x < bytes && (((intptr_t)first_byte+x) & 0x03); ++x) {
                first_byte[x] = uint8_t(start_value);
            }

            // align to 64 byte cacheline boundary
            for (; x < (bytes-3) && (((intptr_t)first_byte+x) & 0x3f); x+=4) {
                *((uint32_t *)(first_byte + x + 0)) = longValue;
            }
            
            // fill 64 byte cacheline at a time
            for ( ; x < (bytes - 63); x += 64) {
                *((uint32_t *)(first_byte + x + 0)) = longValue;
                *((uint32_t *)(first_byte + x + 4)) = longValue;
                *((uint32_t *)(first_byte + x + 8)) = longValue;
                *((uint32_t *)(first_byte + x + 12)) = longValue;
                *((uint32_t *)(first_byte + x + 16)) = longValue;
                *((uint32_t *)(first_byte + x + 20)) = longValue;
                *((uint32_t *)(first_byte + x + 24)) = longValue;
                *((uint32_t *)(first_byte + x + 28)) = longValue;
                *((uint32_t *)(first_byte + x + 32)) = longValue;
                *((uint32_t *)(first_byte + x + 36)) = longValue;
                *((uint32_t *)(first_byte + x + 40)) = longValue;
                *((uint32_t *)(first_byte + x + 44)) = longValue;
                *((uint32_t *)(first_byte + x + 48)) = longValue;
                *((uint32_t *)(first_byte + x + 52)) = longValue;
                *((uint32_t *)(first_byte + x + 56)) = longValue;
                *((uint32_t *)(first_byte + x + 60)) = longValue;
            }
            
            // fill leftover 4 byte words
            for ( ; x < (bytes - 3); x += 4) {
                *((uint32_t *)(first_byte + x + 0)) = longValue;
            }
        }
        
        // fill remaining bytes with simple byte loop
        for ( ; x < bytes; ++x) {
            first_byte[x] = start_value;
        }
    }

    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
    
    size_t zero = 0;
    check_sum( accumulate(first_byte, last_byte, zero), bytes, label );
}

/******************************************************************************/

// this really only works for bytes, but could be generalized for 16 and 32 bit values
template <typename T >
void test_forloop_fill_64(T *first, int count, const uint8_t start_value, const std::string label) {
    int i;
    int bytes = count * sizeof(T);
    uint8_t *first_byte = (uint8_t *)first;
    uint8_t *last_byte = first_byte + bytes;
    
    start_timer();
    
    for(i = 0; i < iterations; ++i) {
        int x = 0;
        
        if (bytes > 128) {
            uint64_t longValue = (uint64_t(start_value) << 8) | start_value;
            longValue |= (longValue << 16);
            longValue |= (longValue << 32);

            // align to 8 byte word boundary
            for (; x < bytes && (((intptr_t)first_byte+x) & 0x07); ++x) {
                first_byte[x] = uint8_t(start_value);
            }

            // fill as many 64 bit words as possible
            for ( ; x < (bytes - 31); x += 32) {
                *((uint64_t *)(first_byte + x + 0)) = longValue;
                *((uint64_t *)(first_byte + x + 8)) = longValue;
                *((uint64_t *)(first_byte + x + 16)) = longValue;
                *((uint64_t *)(first_byte + x + 24)) = longValue;
            }
            
            // fill leftover 8 byte words
            for ( ; x < (bytes - 7); x += 8) {
                *((uint64_t *)(first_byte + x + 0)) = longValue;
            }
        }
        
        // fill remaining bytes with simple byte loop
        for ( ; x < bytes; ++x) {
            first_byte[x] = start_value;
        }
    }

    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
    
    size_t zero = 0;
    check_sum( accumulate(first_byte, last_byte, zero), bytes, label );
}

/******************************************************************************/

// this really only works for bytes, but could be generalized for 16 and 32 bit values
template <typename T >
void test_forloop_fill_64cacheline(T *first, int count, const uint8_t start_value, const std::string label) {
    int i;
    int bytes = count * sizeof(T);
    uint8_t *first_byte = (uint8_t *)first;
    uint8_t *last_byte = first_byte + bytes;
    
    start_timer();
    
    for(i = 0; i < iterations; ++i) {
        int x = 0;
        
        if (bytes > 128) {
            uint64_t longValue = (uint64_t(start_value) << 8) | start_value;
            longValue |= (longValue << 16);
            longValue |= (longValue << 32);

            // align to 8 byte word boundary
            for (; x < bytes && (((intptr_t)first_byte+x) & 0x07); ++x) {
                first_byte[x] = uint8_t(start_value);
            }
            
            // align to 64 byte cacheline boundary
            for (; x < (bytes - 7) && (((intptr_t)first_byte+x) & 0x3f); x += 8) {
                *((uint64_t *)(first_byte + x + 0)) = longValue;
            }
            
            // fill a 64 byte cacheline at a time
            for ( ; x < (bytes - 64); x += 64) {
                *((uint64_t *)(first_byte + x + 0)) = longValue;
                *((uint64_t *)(first_byte + x + 8)) = longValue;
                *((uint64_t *)(first_byte + x + 16)) = longValue;
                *((uint64_t *)(first_byte + x + 24)) = longValue;
                *((uint64_t *)(first_byte + x + 32)) = longValue;
                *((uint64_t *)(first_byte + x + 40)) = longValue;
                *((uint64_t *)(first_byte + x + 48)) = longValue;
                *((uint64_t *)(first_byte + x + 56)) = longValue;
            }
            
            // fill leftover 8 byte words
            for ( ; x < (bytes - 7); x += 8) {
                *((uint64_t *)(first_byte + x + 0)) = longValue;
            }
        }
        
        // fill remaining bytes with simple byte loop
        for ( ; x < bytes; ++x) {
            first_byte[x] = start_value;
        }
    }

    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
    
    size_t zero = 0;
    check_sum( accumulate(first_byte, last_byte, zero), bytes, label );
}

/******************************************************************************/
/******************************************************************************/

template <typename T, typename Filler >
void test_memset_sizes(T *dest, int max_count, Filler fill_test, const std::string label) {
    int i, j;
    const int saved_iterations = iterations;
    
    printf("\ntest   description   absolute   operations\n");
    printf(  "number               time       per second\n\n");
    
    
    // First make sure memory is really allocated and available.
    // This should help avoid overhead of virtual allocation and TLB misses.
    memset(dest, 0x42, max_count );

    for( i = 1, j = 0; i <= max_count; i *= 2, ++j ) {
        
        int64_t temp1 = SIZE / i;
        int64_t temp2 = saved_iterations * temp1;

        if (temp2 > 0x70000000)
            temp2 = 0x70000000;
        if (temp2 < 4)
            temp2 = 4;
        
        iterations = temp2;
        
        fill_test( dest, i, init_value, label );
        
        double millions = ((double)(i) * iterations)/1000000.0;
        
        printf("%2i \"%s %d bytes\"  %5.2f sec   %5.2f M\n",
                j,
                label.c_str(),
                i,
                results[0].time,
                millions/results[0].time );
    
        // reset the test counter
        current_test = 0;
    }

    iterations = saved_iterations;
}

/******************************************************************************/
/******************************************************************************/

template< typename T >
void TestOneType()
{
    gLabels.clear();
    
    std::string myTypeName( getTypeName<T>() );

    T *data = new T[SIZE/sizeof(T)];
    
    test_library_memset( data, SIZE/sizeof(T), init_value, myTypeName + " memset");
    test_std_fill( data, SIZE/sizeof(T), init_value, myTypeName + " std::fill");
    test_iterator_fill( data, SIZE/sizeof(T), init_value, myTypeName + " iterator fill");
    test_forloop_fill( data, SIZE/sizeof(T), init_value, myTypeName + " for loop fill");
    test_forloop_fill_unrolled( data, SIZE/sizeof(T), init_value, myTypeName + " for loop unrolled fill");
    test_forloop_fill_32( data, SIZE/sizeof(T), init_value, myTypeName + " for loop 32bit fill");
    test_forloop_fill_64( data, SIZE/sizeof(T), init_value, myTypeName + " for loop 64bit fill");
    test_forloop_fill_32cacheline( data, SIZE/sizeof(T), init_value, myTypeName + " for loop 32bit cacheline fill");
    test_forloop_fill_64cacheline( data, SIZE/sizeof(T), init_value, myTypeName + " for loop 64bit cacheline fill");


    delete[] data;
    
    std::string temp1( myTypeName + " memset" );
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
    if (argc > 2) init_value = (uint8_t) atof(argv[2]);


    // test basic routines
    TestOneType<uint8_t>();
    TestOneType<uint16_t>();
    TestOneType<uint32_t>();
    TestOneType<uint64_t>();


    // test performance at different sizes of array/fill
    uint8_t *data = new uint8_t[SIZE];
    
    test_memset_sizes( data, SIZE/sizeof(uint8_t), test_library_memset<uint8_t>, "memset" );
    test_memset_sizes( data, SIZE/sizeof(uint8_t), test_std_fill<uint8_t>, "std::fill" );
    test_memset_sizes( data, SIZE/sizeof(uint8_t), test_iterator_fill<uint8_t>, "iterator fill" );
    test_memset_sizes( data, SIZE/sizeof(uint8_t), test_forloop_fill<uint8_t>, "for loop fill" );
    test_memset_sizes( data, SIZE/sizeof(uint8_t), test_forloop_fill_unrolled<uint8_t>, "for loop unrolled fill" );
    test_memset_sizes( data, SIZE/sizeof(uint8_t), test_forloop_fill_32<uint8_t>, "for loop 32bit fill" );
    test_memset_sizes( data, SIZE/sizeof(uint8_t), test_forloop_fill_64<uint8_t>, "for loop 64bit fill" );
    test_memset_sizes( data, SIZE/sizeof(uint8_t), test_forloop_fill_32cacheline<uint8_t>, "for loop 32bit cacheline fill" );
    test_memset_sizes( data, SIZE/sizeof(uint8_t), test_forloop_fill_64cacheline<uint8_t>, "for loop 64bit cacheline fill" );

    delete[] data;
    

    return 0;
}

// the end
/******************************************************************************/
/******************************************************************************/
