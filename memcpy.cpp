/*
    Copyright 2008-2009 Adobe Systems Incorporated
    Copyright 2018-2019 Chris Cox
    Distributed under the MIT License (see accompanying file LICENSE_1_0_0.txt
    or a copy at http://stlab.adobe.com/licenses.html )


Goal:  Test compiler optimizations related to memcpy and hand coded memcpy loops.

Assumptions:

    1) The library functions memcpy and memmove should be optimized for small, medium, and large buffers.
        ie: low overhead for smaller buffer, highly hinted for large buffers.

    2) The STL functions std::copy, std::move, std::copybackward, and std::movebackward
       should be optimized for small, medium, and large buffers.
        ie: low overhead for smaller buffers, highly hinted for large buffers.

    3) The compiler will recognize memcpy like loops and optimize appropriately.
        This could be subtitution of calls to memcpy,
          or it could be just optimizing the loop to get the best throughput.
        On modern systems, cache hinting is usually required for best throughput.



NOTE - On some OSes, memcpy and memmove call into the VM system to set shared pages
        thus running faster than the DRAM bandwidth would allow on large arrays.
       However, on those OSes, calling memcpy can hit mutexes and slow down
        significantly when called from threads.


TODO - Duff's device is generally slower than a simple byte copy loop for small counts
    need to study assembly to find out why.

*/

#include "benchmark_stdint.hpp"
#include <cstddef>
#include <cstdio>
#include <ctime>
#include <cstdlib>
#include <cmath>
#include <algorithm>
#include <string>
#include <deque>
#include "benchmark_results.h"
#include "benchmark_timer.h"
#include "benchmark_algorithms.h"
#include "benchmark_typenames.h"

/******************************************************************************/

// this constant may need to be adjusted to give reasonable minimum times
// For best results, times should be about 1.0 sources for the minimum test run
int iterations = 50;


// 64 Megabytes, intended to be larger than L2 cache on common CPUs
const int SIZE = 64*1024*1024L;


// initial value for filling our arrays, may be changed from the command line
uint8_t init_value = 3;

/******************************************************************************/
/******************************************************************************/

struct iterator_memcpy {

    void operator()( void *dest, const void *source, size_t bytes ) const {
        uint8_t *dest_byte = (uint8_t *)dest;
        const uint8_t *source_byte = (const uint8_t *)source;
        uint8_t *dest_end = dest_byte + bytes;
        
        while (dest_byte != dest_end) {
            *dest_byte = *source_byte;
            ++dest_byte;
            ++source_byte;
        }
    }

};

/******************************************************************************/

struct forloop_memcpy {

    void operator()( void *dest, const void *source, size_t bytes ) const {
        uint8_t *dest_byte = (uint8_t *)dest;
        const uint8_t *source_byte = (const uint8_t *)source;
        int x;
        
        for (x = 0; x < bytes; ++x)
            dest_byte[x] = source_byte[x];
        
    }

};

/******************************************************************************/

struct forloop_unroll_memcpy {

    void operator()( void *dest, const void *source, size_t in_bytes ) const {
        uint8_t *dest_byte = (uint8_t *)dest;
        const uint8_t *source_byte = (const uint8_t *)source;
        ptrdiff_t x;
        
        if (dest == source || in_bytes == 0)
            return;
        
        ptrdiff_t bytes = ptrdiff_t( in_bytes );
        
        for ( x = 0; x < (bytes - 3); x += 4 ) {
            uint8_t src0 = source_byte[x+0];
            uint8_t src1 = source_byte[x+1];
            uint8_t src2 = source_byte[x+2];
            uint8_t src3 = source_byte[x+3];
            dest_byte[x+0] = src0;
            dest_byte[x+1] = src1;
            dest_byte[x+2] = src2;
            dest_byte[x+3] = src3;
        }
        
        // test remaining bytes
        for ( ; x < bytes; ++x )
            dest_byte[x] = source_byte[x];
        
    }

};

/******************************************************************************/

struct forloop_unroll32_memcpy {

    void operator()( void *dest, const void *source, size_t in_bytes ) const {
        uint8_t *dest_byte = (uint8_t *)dest;
        const uint8_t *source_byte = (const uint8_t *)source;
        
        if (dest == source || in_bytes == 0)
            return;
        
        ptrdiff_t bytes = ptrdiff_t( in_bytes );
        
        ptrdiff_t x = 0;
        
        // see if the pointers have the same relative alignment
        if ( bytes > 64 ) {
        
            // align to 32 bit boundary
            for (; x < bytes && ((intptr_t)dest_byte & 0x03); ++x)
                dest_byte[x] = source_byte[x];
            
            // copy 32 bit words
            for (; x < (bytes - 15); x += 16) {
                uint32_t src0 = *((uint32_t *)(source_byte + x + 0));
                uint32_t src4 = *((uint32_t *)(source_byte + x + 4));
                uint32_t src8 = *((uint32_t *)(source_byte + x + 8));
                uint32_t src12 = *((uint32_t *)(source_byte + x + 12));
                *((uint32_t *)(dest_byte + x + 0)) = src0;
                *((uint32_t *)(dest_byte + x + 4)) = src4;
                *((uint32_t *)(dest_byte + x + 8)) = src8;
                *((uint32_t *)(dest_byte + x + 12)) = src12;
            }
            
        }
        
        // copy remaining bytes
        for ( ; x < bytes; ++x )
            dest_byte[x] = source_byte[x];

    }

};

/******************************************************************************/

struct forloop_unroll64_memcpy {

    void operator()( void * dest, const void *source, size_t in_bytes ) const {
        uint8_t *dest_byte = (uint8_t *)dest;
        const uint8_t *source_byte = (const uint8_t *)source;
        
        ptrdiff_t bytes = ptrdiff_t( in_bytes );
        
        ptrdiff_t x = 0;
        
        if ( bytes > 64 ) {
        
            if (dest == source)
                return;
        
            // align to 64 bit boundary
            for (; x < bytes && (((intptr_t)dest_byte+x) & 0x07); ++x)
                dest_byte[x] = source_byte[x];
            
            // copy 64 bit words
            for (; x < (bytes - 31); x += 32) {
                uint64_t src0 = *((uint64_t *)(source_byte + x + 0));
                uint64_t src8 = *((uint64_t *)(source_byte + x + 8));
                uint64_t src16 = *((uint64_t *)(source_byte + x + 16));
                uint64_t src24 = *((uint64_t *)(source_byte + x + 24));
                *((uint64_t *)(dest_byte + x + 0)) = src0;
                *((uint64_t *)(dest_byte + x + 8)) = src8;
                *((uint64_t *)(dest_byte + x + 16)) = src16;
                *((uint64_t *)(dest_byte + x + 24)) = src24;
            }
            
        }
        
        // copy remaining bytes
        for ( ; x < bytes; ++x )
            dest_byte[x] = source_byte[x];
    
    }

};

/******************************************************************************/

struct forloop_unroll64_cacheline_memcpy {

    void operator()( void * dest, const void *source, size_t in_bytes ) const {
        uint8_t *dest_byte = (uint8_t *)dest;
        const uint8_t *source_byte = (const uint8_t *)source;
        
        ptrdiff_t bytes = ptrdiff_t( in_bytes );
        
        ptrdiff_t x = 0;
        
        if (bytes < 64) {
            // simple loop for small counts
            for ( ; x < bytes; ++x )
                dest_byte[x] = source_byte[x];
            return;
        }
        else
        {
            if (source == dest)
                return;
            
            // align to 64 bit boundary on dest
            for (; x < bytes && (((intptr_t)dest_byte+x) & 0x07); ++x)
                dest_byte[x] = source_byte[x];

#if 0
// this should help, but doesn't on Intel i7
            // align to 64 byte cacheline boundary on dest
            for (; x < (bytes - 7) && (((intptr_t)dest_byte+x) & 0x3f); x += 8) {
                uint64_t src0 = *((uint64_t *)(source_byte + x + 0));
                *((uint64_t *)(dest_byte + x + 0)) = src0;
            }
#endif

            // copy 64 bytes cachelines
            for (; x < (bytes - 63); x += 64) {
                uint64_t src0 = *((uint64_t *)(source_byte + x + 0));
                uint64_t src8 = *((uint64_t *)(source_byte + x + 8));
                uint64_t src16 = *((uint64_t *)(source_byte + x + 16));
                uint64_t src24 = *((uint64_t *)(source_byte + x + 24));
                uint64_t src32 = *((uint64_t *)(source_byte + x + 32));
                uint64_t src40 = *((uint64_t *)(source_byte + x + 40));
                uint64_t src48 = *((uint64_t *)(source_byte + x + 48));
                uint64_t src56 = *((uint64_t *)(source_byte + x + 56));
                *((uint64_t *)(dest_byte + x + 0)) = src0;
                *((uint64_t *)(dest_byte + x + 8)) = src8;
                *((uint64_t *)(dest_byte + x + 16)) = src16;
                *((uint64_t *)(dest_byte + x + 24)) = src24;
                *((uint64_t *)(dest_byte + x + 32)) = src32;
                *((uint64_t *)(dest_byte + x + 40)) = src40;
                *((uint64_t *)(dest_byte + x + 48)) = src48;
                *((uint64_t *)(dest_byte + x + 56)) = src56;
            }
            
            // copy remaining bytes
            for ( ; x < bytes; ++x )
                dest_byte[x] = source_byte[x];
            
        }

        
    }

};

/******************************************************************************/

struct std_copy {

    void operator()( void *dest, const void *source, size_t bytes ) const {
        uint8_t *dest_byte = (uint8_t *)dest;
        const uint8_t *source_byte = (const uint8_t *)source;
        const uint8_t *source_end = source_byte + bytes;
        std::copy( source_byte, source_end, dest_byte );
    }

};

/******************************************************************************/

struct std_move {

    void operator()( void *dest, const void *source, size_t bytes ) const {
        uint8_t *dest_byte = (uint8_t *)dest;
        const uint8_t *source_byte = (const uint8_t *)source;
        const uint8_t *source_end = source_byte + bytes;
        std::move( source_byte, source_end, dest_byte );
    }

};

/******************************************************************************/

struct std_copybackward {

    void operator()( void *dest, const void *source, size_t bytes ) const {
        uint8_t *dest_byte = (uint8_t *)dest + bytes;
        const uint8_t *source_byte = (const uint8_t *)source;
        const uint8_t *source_end = source_byte + bytes;
        std::copy_backward( source_byte, source_end, dest_byte );
    }

};

/******************************************************************************/

struct std_movebackward {

    void operator()( void *dest, const void *source, size_t bytes ) const {
        uint8_t *dest_byte = (uint8_t *)dest + bytes;
        const uint8_t *source_byte = (const uint8_t *)source;
        const uint8_t *source_end = source_byte + bytes;
        std::move_backward( source_byte, source_end, dest_byte );
    }

};

/******************************************************************************/
/******************************************************************************/

template <typename T, typename Move >
void test_memcpy(T *dest, const T *source, int count, Move copier, const char *label) {
    int i;
    int bytes = count * sizeof(T);
    
    // no overlap between buffers, so this is safe
    memset(dest,0,bytes);

    start_timer();

    for(i = 0; i < iterations; ++i) {
        copier( dest, source, bytes );
    }
    
    if ( memcmp(dest, source, bytes) != 0 )
        printf("test %s failed\n", label);
    
    // Yep, I ran into a bad implementation on one OS - now fixed.
    if (current_test < 0 || current_test > 100) {
        printf("**FATAL** Heap corrupted current_test is %d\n", current_test);
        exit(-4);
    }
    
    if (allocated_results < 0 || allocated_results > 100) {
        printf("**FATAL** Heap corrupted allocated_results is %d\n", allocated_results);
        exit(-2);
    }
    
    record_result( timer(), label );
}

/******************************************************************************/

template <typename T, typename Copier >
void test_memcpy_sizes(T *dest, const T *source, int max_count, Copier copy_test, const char *label) {
    int i, j;
    const int saved_iterations = iterations;
    
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
        
        test_memcpy( dest, source, i, copy_test, label );
        
        double millions = ((double)(i) * iterations)/1000000.0;
        
        printf("%2i \"%s %d bytes\"  %5.2f sec   %5.2f M\n",
                j,
                label,
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

const int alignment_pad = 1024;
uint8_t data8u_source[SIZE];
uint8_t data8u[SIZE + alignment_pad];

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
    fill_random( data8u, data8u+SIZE+alignment_pad );
    fill_random( data8u_source, data8u_source+SIZE );



    test_memcpy( data8u, data8u_source, SIZE, memcpy, "memcpy");
    test_memcpy( data8u, data8u_source, SIZE, memmove, "memmove");
    test_memcpy( data8u, data8u_source, SIZE, std_copy(), "std::copy");
    test_memcpy( data8u, data8u_source, SIZE, std_move(), "std::move");
    test_memcpy( data8u, data8u_source, SIZE, std_copybackward(), "std::copybackward");
    test_memcpy( data8u, data8u_source, SIZE, std_movebackward(), "std::movebackward");
    test_memcpy( data8u, data8u_source, SIZE, iterator_memcpy(), "iterator copy");
    test_memcpy( data8u, data8u_source, SIZE, forloop_memcpy(), "for loop copy");
    test_memcpy( data8u, data8u_source, SIZE, forloop_unroll_memcpy(), "for loop unroll copy");
    test_memcpy( data8u, data8u_source, SIZE, forloop_unroll32_memcpy(), "for loop unroll32 copy");
    test_memcpy( data8u, data8u_source, SIZE, forloop_unroll64_memcpy(), "for loop unroll64 copy");
    test_memcpy( data8u, data8u_source, SIZE, forloop_unroll64_cacheline_memcpy(), "for loop unroll64 cacheline copy");
    
    summarize("memcpy", SIZE, iterations, kDontShowGMeans, kDontShowPenalty );



    // aligned buffers
    uint8_t *dest = data8u;
    test_memcpy_sizes( dest, data8u_source, SIZE, memcpy, "memcpy aligned");
    test_memcpy_sizes( dest, data8u_source, SIZE, memmove, "memmove aligned");
    test_memcpy_sizes( dest, data8u_source, SIZE, std_copy(), "std::copy aligned");
    test_memcpy_sizes( dest, data8u_source, SIZE, std_move(), "std::move aligned");
    test_memcpy_sizes( dest, data8u_source, SIZE, std_copybackward(), "std::copybackward aligned");
    test_memcpy_sizes( dest, data8u_source, SIZE, std_movebackward(), "std::movebackward aligned");
    test_memcpy_sizes( dest, data8u_source, SIZE, iterator_memcpy(), "iterator copy aligned");
    test_memcpy_sizes( dest, data8u_source, SIZE, forloop_memcpy(), "for loop copy aligned");
    test_memcpy_sizes( dest, data8u_source, SIZE, forloop_unroll_memcpy(), "for loop unroll copy aligned");
    test_memcpy_sizes( dest, data8u_source, SIZE, forloop_unroll32_memcpy(), "for loop unroll32 copy aligned");
    test_memcpy_sizes( dest, data8u_source, SIZE, forloop_unroll64_memcpy(), "for loop unroll64 copy aligned");
    test_memcpy_sizes( dest, data8u_source, SIZE, forloop_unroll64_cacheline_memcpy(), "for loop unroll64 cacheline copy aligned");
    
    
    // unaligned buffers
    dest = data8u + 3;
    test_memcpy_sizes( dest, data8u_source, SIZE, memcpy, "memcpy unaligned");
    test_memcpy_sizes( dest, data8u_source, SIZE, memmove, "memmove unaligned");
    test_memcpy_sizes( dest, data8u_source, SIZE, std_copy(), "std::copy unaligned");
    test_memcpy_sizes( dest, data8u_source, SIZE, std_move(), "std::move unaligned");
    test_memcpy_sizes( dest, data8u_source, SIZE, std_copybackward(), "std::copybackward unaligned");
    test_memcpy_sizes( dest, data8u_source, SIZE, std_movebackward(), "std::movebackward unaligned");
    test_memcpy_sizes( dest, data8u_source, SIZE, iterator_memcpy(), "iterator copy unaligned");
    test_memcpy_sizes( dest, data8u_source, SIZE, forloop_memcpy(), "for loop copy unaligned");
    test_memcpy_sizes( dest, data8u_source, SIZE, forloop_unroll_memcpy(), "for loop unroll copy unaligned");
    test_memcpy_sizes( dest, data8u_source, SIZE, forloop_unroll32_memcpy(), "for loop unroll32 copy unaligned");
    test_memcpy_sizes( dest, data8u_source, SIZE, forloop_unroll64_memcpy(), "for loop unroll64 copy unaligned");
    test_memcpy_sizes( dest, data8u_source, SIZE, forloop_unroll64_cacheline_memcpy(), "for loop unroll64 cacheline copy unaligned");


    return 0;
}

// the end
/******************************************************************************/
/******************************************************************************/
