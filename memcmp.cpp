/*
    Copyright 2008-2009 Adobe Systems Incorporated
    Copyright 2018 Chris Cox
    Distributed under the MIT License (see accompanying file LICENSE_1_0_0.txt
    or a copy at http://stlab.adobe.com/licenses.html )


Goal:  Test compiler optimizations related to memcmp and hand coded memcmp loops.


Assumptions:

    1) The compiler will recognize memcmp like loops and optimize appropriately.
        This could be subtitution of calls to memcmp,
         or it could be just optimizing the loop to get the best throughput.
        On modern systems, cache hinting is usually required for best throughput.

    2) The library function memcmp should be optimized for small, medium, and large buffers.
        ie: low overhead for smaller buffer, highly hinted for large buffers.

    3) The STL functions equal and mismatch should be optimized for small, medium, and large buffers.
        ie: low overhead for smaller buffers, highly hinted for large buffers.




NOTE - on some OSes, memcmp calls into the VM system to test for shared pages
        thus running faster than the DRAM bandwidth would allow on large arrays
        
        However, on those OSes, calling memcmp can hit mutexes and slow down
        significantly when called from threads.


NOTE - Linux memcmp returns 0, +-1 instead of the actual difference
NOTE - and sometimes Linux memcmp returns 0, +-256 instead of the actual difference


TODO - test performance of unaligned buffers
*/

#include "benchmark_stdint.hpp"
#include <cstddef>
#include <cstdio>
#include <ctime>
#include <cstdlib>
#include <cmath>
#include <string>
#include <deque>
#include <algorithm>
#include "benchmark_results.h"
#include "benchmark_timer.h"
#include "benchmark_algorithms.h"

/******************************************************************************/

// this constant may need to be adjusted to give reasonable minimum times
// For best results, times should be about 1.0 seconds for the minimum test run
int iterations = 70;


// 64 Megabytes, intended to be larger than L2 cache on common CPUs
// needs to be divisible by 8
const int SIZE = 64*1024*1024L;


// initial value for filling our arrays, may be changed from the command line
uint8_t init_value = 3;

/******************************************************************************/
/******************************************************************************/

struct stdequal{

    int operator()( const void *first, const void *second, size_t bytes ) const {
        const uint8_t *first_byte = (const uint8_t *)first;
        const uint8_t *second_byte = (const uint8_t *)second;
        const uint8_t *first_end = first_byte + bytes;
        
        return !std::equal( first_byte, first_end, second_byte );
    }

};

/******************************************************************************/

struct stdmismatch{

    int operator()( const void *first, const void *second, size_t bytes ) const {
        const uint8_t *first_byte = (const uint8_t *)first;
        const uint8_t *second_byte = (const uint8_t *)second;
        const uint8_t *first_end = first_byte + bytes;
        
        auto result = std::mismatch( first_byte, first_end, second_byte );
        return (result.first != first_end);
    }

};

/******************************************************************************/

struct forloop_memcmp {

    int operator()( const void *first, const void *second, size_t bytes ) const {
        const uint8_t *first_byte = (const uint8_t *)first;
        const uint8_t *second_byte = (const uint8_t *)second;
        int x;
        
        for (x = 0; x < bytes; ++x) {
            if (first_byte[x] != second_byte[x]) {
                return (first_byte[x] - second_byte[x]);
            }
        }
        
        return 0;
    }

};

/******************************************************************************/

struct forloop_memcmp2 {

    int operator()( const void *first, const void *second, size_t bytes ) const {
        const uint8_t *first_byte = (const uint8_t *)first;
        const uint8_t *second_byte = (const uint8_t *)second;
        int x;
        
        for (x = 0; x < bytes; ++x) {
            if (first_byte[x] != second_byte[x])
                return 1;
        }
        
        return 0;
    }

};

/******************************************************************************/

struct iterator_memcmp {

    int operator()( const void *first, const void *second, size_t bytes ) const {
        const uint8_t *first_byte = (const uint8_t *)first;
        const uint8_t *second_byte = (const uint8_t *)second;
        const uint8_t *first_end = first_byte + bytes;
        
        while (first_byte != first_end) {
            if (*first_byte != *second_byte) {
                return (*first_byte - *second_byte);
            }
            ++first_byte;
            ++second_byte;
        }
        
        return 0;
    }

};

/******************************************************************************/

struct iterator_memcmp2 {

    int operator()( const void *first, const void *second, size_t bytes ) const {
        const uint8_t *first_byte = (const uint8_t *)first;
        const uint8_t *second_byte = (const uint8_t *)second;
        const uint8_t *first_end = first_byte + bytes;
        
        for ( ; first_byte != first_end; ++first_byte, ++second_byte) {
            if (*first_byte != *second_byte) {
                return (*first_byte - *second_byte);
            }
        }
        
        return 0;
    }

};

/******************************************************************************/

struct iterator_memcmp3 {

    int operator()( const void *first, const void *second, size_t bytes ) const {
        const uint8_t *first_byte = (const uint8_t *)first;
        const uint8_t *second_byte = (const uint8_t *)second;
        const uint8_t *first_end = first_byte + bytes;
        
        while (first_byte != first_end) {
            if (*first_byte != *second_byte) {
                return 1;
            }
            ++first_byte;
            ++second_byte;
        }
        
        return 0;
    }

};

/******************************************************************************/

struct forloop_unroll_memcmp {

    int operator()( const void *first, const void *second, size_t in_bytes ) const {
        const uint8_t *first_byte = (const uint8_t *)first;
        const uint8_t *second_byte = (const uint8_t *)second;
        ptrdiff_t x;
        
        ptrdiff_t bytes = ptrdiff_t( in_bytes );
        
        for (x = 0; x < (bytes - 3); x += 4) {
            if (first_byte[x+0] != second_byte[x+0]) {
                return (first_byte[x+0] - second_byte[x+0]);
            }
            if (first_byte[x+1] != second_byte[x+1]) {
                return (first_byte[x+1] - second_byte[x+1]);
            }
            if (first_byte[x+2] != second_byte[x+2]) {
                return (first_byte[x+2] - second_byte[x+2]);
            }
            if (first_byte[x+3] != second_byte[x+3]) {
                return (first_byte[x+3] - second_byte[x+3]);
            }
        }
        
        // test remaining bytes
        for ( ; x < bytes; ++x ) {
            if (first_byte[x] != second_byte[x]) {
                return (first_byte[x] - second_byte[x]);
            }
        }
        
        return 0;
    }

};

/******************************************************************************/

struct forloop_unroll2_memcmp {

    int operator()( const void *first, const void *second, size_t in_bytes ) const {
        const uint8_t *first_byte = (const uint8_t *)first;
        const uint8_t *second_byte = (const uint8_t *)second;
        ptrdiff_t x;
        
        ptrdiff_t bytes = ptrdiff_t( in_bytes );
        
        for (x = 0; x < (bytes - 3); x += 4) {
            if (   first_byte[x+0] != second_byte[x+0]
                || first_byte[x+1] != second_byte[x+1]
                || first_byte[x+2] != second_byte[x+2]
                || first_byte[x+3] != second_byte[x+3])
                    break;
        }
        
        // test remaining bytes
        for ( ; x < bytes; ++x ) {
            if (first_byte[x] != second_byte[x]) {
                return (first_byte[x] - second_byte[x]);
            }
        }
        
        return 0;
    }

};

/******************************************************************************/

struct forloop_unroll32_memcmp {

    int operator()( const void *first, const void *second, size_t in_bytes ) const {
        const uint8_t *first_byte = (const uint8_t *)first;
        const uint8_t *second_byte = (const uint8_t *)second;
        
        ptrdiff_t bytes = ptrdiff_t( in_bytes );
        
        ptrdiff_t x = 0;
        
        if (in_bytes >= 32) {
            
            // NOTE - even with unaligned buffers, larger word compares are much faster
            // aligned = 5.8GB/s, unaligned = 5.8GB/s, byte loop = 1.5GB/s
            // DRAM bandwidth is twice the operations/s (so 11.6 GB/s)
    
            // align to 32 bit boundary
            for (; x < bytes && (((intptr_t)first_byte+x) & 0x03); ++x) {
                if (first_byte[x] != second_byte[x]) {
                    return (first_byte[x] - second_byte[x]);
                }
            }
            
            // test 32 bit words
            for (; x < (bytes - 15); x += 16) {
                if (   *((uint32_t *)(first_byte + x + 0)) != *((uint32_t *)(second_byte + x + 0))
                    || *((uint32_t *)(first_byte + x + 4)) != *((uint32_t *)(second_byte + x + 4))
                    || *((uint32_t *)(first_byte + x + 8)) != *((uint32_t *)(second_byte + x + 8))
                    || *((uint32_t *)(first_byte + x + 12)) != *((uint32_t *)(second_byte + x + 12)) )
                        break;
            }
        }
        
        // test remaining bytes
        for ( ; x < bytes; ++x ) {
            if (first_byte[x] != second_byte[x]) {
                return (first_byte[x] - second_byte[x]);
            }
        }
        
        return 0;
    }

};

/******************************************************************************/

struct forloop_unroll64_memcmp {

    int operator()( const void *first, const void *second, size_t in_bytes ) const {
        const uint8_t *first_byte = (const uint8_t *)first;
        const uint8_t *second_byte = (const uint8_t *)second;
        
        ptrdiff_t bytes = ptrdiff_t( in_bytes );
        
        ptrdiff_t x = 0;
        
        if (in_bytes >= 32) {
            
            // NOTE - even with unaligned buffers, larger word compares are much faster
            // aligned = 12.1GB/s, unaligned = 11.4GB/s, byte loop = 1.5GB/s
            // DRAM bandwidth is twice the operations/s (so 24.2 GB/s)
            
            // align first to 64 bit boundary
            for (; x < bytes && (((intptr_t)first_byte+x) & 0x07); ++x) {
                if (first_byte[x] != second_byte[x]) {
                    return (first_byte[x] - second_byte[x]);
                }
            }
            
            // test 64 bit words
            for (; x < (bytes - 31); x += 32) {
                if (   *((uint64_t *)(first_byte + x + 0)) != *((uint64_t *)(second_byte + x + 0))
                    || *((uint64_t *)(first_byte + x + 8)) != *((uint64_t *)(second_byte + x + 8))
                    || *((uint64_t *)(first_byte + x + 16)) != *((uint64_t *)(second_byte + x + 16))
                    || *((uint64_t *)(first_byte + x + 24)) != *((uint64_t *)(second_byte + x + 24)) )
                        break;
            }
        }
        
        // test remaining bytes
        for ( ; x < bytes; ++x ) {
            if (first_byte[x] != second_byte[x]) {
                return (first_byte[x] - second_byte[x]);
            }
        }
        
        return 0;
    }

};

/******************************************************************************/

struct forloop_unroll64_cacheline_memcmp {

    int operator()( const void *first, const void *second, size_t in_bytes ) const {
        const uint8_t *first_byte = (const uint8_t *)first;
        const uint8_t *second_byte = (const uint8_t *)second;
        
        ptrdiff_t bytes = ptrdiff_t( in_bytes );
        
        ptrdiff_t x = 0;
        
        if (in_bytes >= 32) {
            
            // NOTE - even with unaligned buffers, larger word compares are much faster
            // aligned = 12.1GB/s, unaligned = 11.4GB/s, byte loop = 1.5GB/s
            // DRAM bandwidth is twice the operations/s (so 24.2 GB/s)
            
            // align first to 64 bit boundary
            for (; x < bytes && (((intptr_t)first_byte+x) & 0x07); ++x) {
                if (first_byte[x] != second_byte[x]) {
                    return (first_byte[x] - second_byte[x]);
                }
            }
            
            // align first to 64 byte boundary
            for (; x < (bytes - 7) && (((intptr_t)first_byte+x) & 0x3f); x += 8) {
                if ( *((uint64_t *)(first_byte + x)) != *((uint64_t *)(second_byte + x)) )
                    goto byte_loop;
            }
            
            // test 64 bit words, across an entire cacheline
            for (; x < (bytes - 63); x += 64) {
                if (   *((uint64_t *)(first_byte + x + 0)) != *((uint64_t *)(second_byte + x + 0))
                    || *((uint64_t *)(first_byte + x + 8)) != *((uint64_t *)(second_byte + x + 8))
                    || *((uint64_t *)(first_byte + x + 16)) != *((uint64_t *)(second_byte + x + 16))
                    || *((uint64_t *)(first_byte + x + 24)) != *((uint64_t *)(second_byte + x + 24))
                    || *((uint64_t *)(first_byte + x + 32)) != *((uint64_t *)(second_byte + x + 32))
                    || *((uint64_t *)(first_byte + x + 40)) != *((uint64_t *)(second_byte + x + 40))
                    || *((uint64_t *)(first_byte + x + 48)) != *((uint64_t *)(second_byte + x + 48))
                    || *((uint64_t *)(first_byte + x + 56)) != *((uint64_t *)(second_byte + x + 56)) )
                        break;
            }
        }

byte_loop:
        // test remaining bytes
        for ( ; x < bytes; ++x ) {
            if (first_byte[x] != second_byte[x]) {
                return (first_byte[x] - second_byte[x]);
            }
        }
        
        return 0;
    }

};

/******************************************************************************/
/******************************************************************************/

static std::deque<std::string> gLabels;

template <typename T, typename Compare >
void test_memcmp(const T *first, const T *second, int count, bool expected_result, Compare comparator, const std::string label) {
    int i;
    int bytes = count * sizeof(T);

    start_timer();

    for(i = 0; i < iterations; ++i) {
        // sigh, Linux memcmp is wonky - some return 1, some return 256
        bool result( comparator( first, second, bytes ) != 0 );
        
        // moving this test out of the loop causes unwanted overoptimization
        if ( result != expected_result )
            printf("test %s by %d failed (got %d instead of %d)\n", label.c_str(), count, (int)result, (int)expected_result );
    }
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

template <typename T, typename Compare >
void test_memcmp_sizes(const T *first, const T *second, int max_count, bool result, Compare copier, const std::string label) {
    int i, j;
    const int saved_iterations = iterations;
    
    gLabels.clear();
    
    printf("\ntest   description   absolute   operations\n");
    printf(  "number               time       per second\n\n");

    for( i = 1, j = 0; i <= max_count; i *= 2, ++j ) {
        
        int64_t temp1 = SIZE / i;
        int64_t temp2 = saved_iterations * temp1;

        if (temp2 > 0x70000000)
            temp2 = 0x70000000;
        if (temp2 < 4)
            temp2 = 4;
        
        iterations = temp2;
        
        test_memcmp( first, second, i, result, copier, label );
        
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

// our global arrays of numbers to be operated upon

uint8_t data8u[SIZE/sizeof(uint8_t)];
const int alignment_pad = 1024;
uint8_t data8u_dest[SIZE/sizeof(uint8_t) + alignment_pad]; // leave some room for alignment testing

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

    gLabels.clear();

    ::fill( data8u, data8u+(SIZE/sizeof(uint8_t)), uint8_t(init_value) );
    ::fill( data8u_dest, data8u_dest+(SIZE/sizeof(uint8_t) + alignment_pad), uint8_t(init_value) );


    test_memcmp( data8u, data8u_dest, SIZE/sizeof(uint8_t), false, memcmp, "uint8_t memcmp same");
    test_memcmp( data8u, data8u_dest, SIZE/sizeof(uint8_t), false, stdequal(), "uint8_t std::equal same");
    test_memcmp( data8u, data8u_dest, SIZE/sizeof(uint8_t), false, stdmismatch(), "uint8_t std::mismatch same");
    test_memcmp( data8u, data8u_dest, SIZE/sizeof(uint8_t), false, iterator_memcmp(), "uint8_t iterator compare same");
    test_memcmp( data8u, data8u_dest, SIZE/sizeof(uint8_t), false, iterator_memcmp2(), "uint8_t iterator2 compare same");
    test_memcmp( data8u, data8u_dest, SIZE/sizeof(uint8_t), false, iterator_memcmp3(), "uint8_t iterator3 compare same");
    test_memcmp( data8u, data8u_dest, SIZE/sizeof(uint8_t), false, forloop_memcmp(), "uint8_t for loop compare same");
    test_memcmp( data8u, data8u_dest, SIZE/sizeof(uint8_t), false, forloop_memcmp2(), "uint8_t for loop2 compare same");
    test_memcmp( data8u, data8u_dest, SIZE/sizeof(uint8_t), false, forloop_unroll_memcmp(), "uint8_t for loop unroll compare same");
    test_memcmp( data8u, data8u_dest, SIZE/sizeof(uint8_t), false, forloop_unroll2_memcmp(), "uint8_t for loop unroll2 compare same");
    test_memcmp( data8u, data8u_dest, SIZE/sizeof(uint8_t), false, forloop_unroll32_memcmp(), "uint8_t for loop unroll32 compare same");
    test_memcmp( data8u, data8u_dest, SIZE/sizeof(uint8_t), false, forloop_unroll64_memcmp(), "uint8_t for loop unroll64 compare same");
    test_memcmp( data8u, data8u_dest, SIZE/sizeof(uint8_t), false, forloop_unroll64_cacheline_memcmp(), "uint8_t for loop unroll64 cacheline compare same");
    
    summarize("uint8_t memcmp same", SIZE, iterations, kDontShowGMeans, kDontShowPenalty );


    bool expected_difference = true;
    data8u[(SIZE/sizeof(uint8_t))-1] += 1;    // last byte in the array
    
    test_memcmp( data8u, data8u_dest, SIZE/sizeof(uint8_t), expected_difference, memcmp, "uint8_t memcmp different");
    test_memcmp( data8u, data8u_dest, SIZE/sizeof(uint8_t), expected_difference, stdequal(), "uint8_t std::equal different");
    test_memcmp( data8u, data8u_dest, SIZE/sizeof(uint8_t), expected_difference, stdmismatch(), "uint8_t std::mismatch different");
    test_memcmp( data8u, data8u_dest, SIZE/sizeof(uint8_t), expected_difference, iterator_memcmp(), "uint8_t iterator compare different");
    test_memcmp( data8u, data8u_dest, SIZE/sizeof(uint8_t), expected_difference, iterator_memcmp2(), "uint8_t iterator2 compare different");
    test_memcmp( data8u, data8u_dest, SIZE/sizeof(uint8_t), expected_difference, iterator_memcmp3(), "uint8_t iterator3 compare different");
    test_memcmp( data8u, data8u_dest, SIZE/sizeof(uint8_t), expected_difference, forloop_memcmp(), "uint8_t for loop compare different");
    test_memcmp( data8u, data8u_dest, SIZE/sizeof(uint8_t), expected_difference, forloop_memcmp2(), "uint8_t for loop2 compare different");
    test_memcmp( data8u, data8u_dest, SIZE/sizeof(uint8_t), expected_difference, forloop_unroll_memcmp(), "uint8_t for loop unroll compare different");
    test_memcmp( data8u, data8u_dest, SIZE/sizeof(uint8_t), expected_difference, forloop_unroll2_memcmp(), "uint8_t for loop unroll2 compare different");
    test_memcmp( data8u, data8u_dest, SIZE/sizeof(uint8_t), expected_difference, forloop_unroll32_memcmp(), "uint8_t for loop unroll32 compare different");
    test_memcmp( data8u, data8u_dest, SIZE/sizeof(uint8_t), expected_difference, forloop_unroll64_memcmp(), "uint8_t for loop unroll64 compare different");
    test_memcmp( data8u, data8u_dest, SIZE/sizeof(uint8_t), expected_difference, forloop_unroll64_cacheline_memcmp(), "uint8_t for loop unroll64 cacheline compare different");
    
    summarize("uint8_t memcmp different", SIZE, iterations, kDontShowGMeans, kDontShowPenalty );



    ::fill( data8u, data8u+(SIZE/sizeof(uint8_t)), uint8_t(init_value) );
    
    // TODO - test performance of unaligned buffers
    uint8_t *dest = data8u_dest;

    test_memcmp_sizes( data8u, dest, SIZE/sizeof(uint8_t), false, memcmp, "memcmp");
    test_memcmp_sizes( data8u, dest, SIZE/sizeof(uint8_t), false, stdequal(), "std::equal");
    test_memcmp_sizes( data8u, dest, SIZE/sizeof(uint8_t), false, stdmismatch(), "std::mismatch");
    test_memcmp_sizes( data8u, dest, SIZE/sizeof(uint8_t), false, iterator_memcmp(), "iterator compare");
    test_memcmp_sizes( data8u, dest, SIZE/sizeof(uint8_t), false, iterator_memcmp2(), "iterator2 compare");
    test_memcmp_sizes( data8u, dest, SIZE/sizeof(uint8_t), false, iterator_memcmp3(), "iterator3 compare");
    test_memcmp_sizes( data8u, dest, SIZE/sizeof(uint8_t), false, forloop_memcmp(), "for loop compare");
    test_memcmp_sizes( data8u, dest, SIZE/sizeof(uint8_t), false, forloop_memcmp2(), "for loop2 compare");
    test_memcmp_sizes( data8u, dest, SIZE/sizeof(uint8_t), false, forloop_unroll_memcmp(), "for loop unroll compare");
    test_memcmp_sizes( data8u, dest, SIZE/sizeof(uint8_t), false, forloop_unroll2_memcmp(), "for loop unroll2 compare");
    test_memcmp_sizes( data8u, dest, SIZE/sizeof(uint8_t), false, forloop_unroll32_memcmp(), "for loop unroll32 compare");
    test_memcmp_sizes( data8u, dest, SIZE/sizeof(uint8_t), false, forloop_unroll64_memcmp(), "for loop unroll64 compare");
    test_memcmp_sizes( data8u, dest, SIZE/sizeof(uint8_t), false, forloop_unroll64_cacheline_memcmp(), "for loop unroll64 cacheline compare");



    return 0;
}

// the end
/******************************************************************************/
/******************************************************************************/
