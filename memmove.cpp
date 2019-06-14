/*
    Copyright 2008-2009 Adobe Systems Incorporated
    Copyright 2018-2019 Chris Cox
    Distributed under the MIT License (see accompanying file LICENSE_1_0_0.txt
    or a copy at http://stlab.adobe.com/licenses.html )


Goal:  Test compiler optimizations related to memmove and hand coded memmove loops.


Assumptions:

    1) The compiler will recognize memmove like loops and optimize appropriately.
        This could be subtitution of calls to memmove,
        or it could be just optimizing the loop to get the best throughput.

    2) The library function memmove should be optimized for small, medium, and large buffers;
        forward and reverse copies.
        ie: low overhead for smaller buffer, highly hinted for large buffers.

    3) The STL function std::move should be optimized for small, medium, and large buffers;
        forward and reverse copies.
        ie: low overhead for smaller buffers, highly hinted for large buffers.



NOTE - On some OSes, memmove calls into the VM system to set shared pages
        thus running faster than the DRAM bandwidth would allow on large arrays.
       However, on those OSes, calling memmove can hit mutexes and slow down
        significantly when called from threads.

NOTE - some of this is duplicated in memcpy.cpp




NOTE - should bcopy() be tested as well?       No.
    Deprecated POISIX.1, supposedly removed, but still available in some distributions.
    <strings.h>
    void bcopy(const void *src, void *dst, size_t len);

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

/******************************************************************************/

// this constant may need to be adjusted to give reasonable minimum times
// For best results, times should be about 1.0 seconds for the minimum test run
int iterations = 50;


// 64 Megabytes, intended to be larger than L2 cache on common CPUs
const int SIZE = 64*1024*1024;


// seed value, may be changed from the command line
uint64_t init_value = 3;

/******************************************************************************/
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

// this seems to be the most common pattern
struct iterator_memmove {

    void * operator()( void *dest, const void *source, size_t bytes ) const {
        if (bytes == 0 || source == dest)
            return dest;
        
        uint8_t *dest_byte = (uint8_t *)dest;
        const uint8_t *source_byte = (const uint8_t *)source;
        uint8_t *dest_end = dest_byte + bytes;
        const uint8_t *source_end = source_byte + bytes;
        
        if (dest_byte < source_byte) {
            // we can safely copy forward
            while (dest_byte != dest_end) {
                *dest_byte = *source_byte;
                ++dest_byte;
                ++source_byte;
            }
        } else {
            // we must copy in reverse
            --dest_end;
            --source_end;
            while (dest_byte != dest_end) {
                *dest_end = *source_end;
                --dest_end;
                --source_end;
            }
        }
        
        return dest;
    }

};

/******************************************************************************/

// slightly smarter
struct iterator_memmove2 {

    void * operator()( void *dest, const void *source, size_t bytes ) const {
        if (dest == source || bytes == 0)
            return dest;
    
        uint8_t *dest_byte = (uint8_t *)dest;
        const uint8_t *source_byte = (const uint8_t *)source;
        uint8_t *dest_end = dest_byte + bytes;
        const uint8_t *source_end = source_byte + bytes;
        
        bool overlap = (dest_byte < source_end);
        
        if (overlap && (source_byte < dest_byte)) {
            // we must copy in reverse
            --dest_end;
            --source_end;
            while (dest_byte != dest_end) {
                *dest_end = *source_end;
                --dest_end;
                --source_end;
            }
        } else {
            // we can safely copy forward
            while (dest_byte != dest_end) {
                *dest_byte = *source_byte;
                ++dest_byte;
                ++source_byte;
            }
        }
        
        return dest;
    }

};

/******************************************************************************/

// this seems to be the most common pattern
struct forloop_memmove {

    void * operator()( void *dest, const void *source, size_t bytes ) const {
        if (bytes == 0 || source == dest)
            return dest;
        
        uint8_t *dest_byte = (uint8_t *)dest;
        const uint8_t *source_byte = (const uint8_t *)source;
        size_t x;

        if (dest_byte <= source_byte) {
            // we can safely copy forward
            for (x = 0; x < bytes; ++x)
                dest_byte[x] = source_byte[x];
        } else {
            // we must copy in reverse
            for (x = 0; x < bytes; ++x)
                dest_byte[bytes-1-x] = source_byte[bytes-1-x];
        }
        
        return dest;
    }

};

/******************************************************************************/

// slightly smarter
struct forloop_memmove2 {

    void * operator()( void *dest, const void *source, size_t bytes ) const {
        if (dest == source || bytes == 0)
            return dest;
        
        uint8_t *dest_byte = (uint8_t *)dest;
        const uint8_t *source_byte = (const uint8_t *)source;
        size_t x;
        
        bool overlap = (dest_byte < (source_byte + bytes));
        
        if (overlap && (source_byte < dest_byte) ) {
            // we must copy in reverse
            for (x = 0; x < bytes; ++x)
                dest_byte[bytes-1-x] = source_byte[bytes-1-x];
        } else {
            // we can safely copy forward
            for (x = 0; x < bytes; ++x)
                dest_byte[x] = source_byte[x];
        }
        
        return dest;
    }

};

/******************************************************************************/

// simple loop unrolling
struct forloop_unroll_memmove {

    void * operator()( void *dest, const void *source, size_t in_bytes ) const {
        if (dest == source || in_bytes == 0)
            return dest;
        
        uint8_t *dest_byte = (uint8_t *)dest;
        const uint8_t *source_byte = (const uint8_t *)source;
        ptrdiff_t x;
        
        bool overlap = (dest_byte < (source_byte + in_bytes));
        
        ptrdiff_t bytes = ptrdiff_t( in_bytes );
        
        if (overlap && (source_byte < dest_byte) ) {
            // we must copy in reverse
            for (x = 0; x < (bytes - 3); x += 4) {
                dest_byte[bytes-1-(x+0)] = source_byte[bytes-1-(x+0)];
                dest_byte[bytes-1-(x+1)] = source_byte[bytes-1-(x+1)];
                dest_byte[bytes-1-(x+2)] = source_byte[bytes-1-(x+2)];
                dest_byte[bytes-1-(x+3)] = source_byte[bytes-1-(x+3)];
            }
            
            for (; x < bytes; ++x)
                dest_byte[bytes-1-x] = source_byte[bytes-1-x];
        } else {
            // we can safely copy forward
            for (x = 0; x < (bytes - 3); x += 4) {
                dest_byte[x+0] = source_byte[x+0];
                dest_byte[x+1] = source_byte[x+1];
                dest_byte[x+2] = source_byte[x+2];
                dest_byte[x+3] = source_byte[x+3];
            }
            
            // copy remaining bytes
            for ( ; x < bytes; ++x )
                dest_byte[x] = source_byte[x];
        }
        
        return dest;
    }

};

/******************************************************************************/

// loop unrolled, and copy 32 bit values whenever possible
struct forloop_unroll32_memmove {

    void * operator()( void *dest, const void *source, size_t in_bytes ) const {
        if (dest == source || in_bytes == 0)
            return dest;
        
        uint8_t *dest_byte = (uint8_t *)dest;
        const uint8_t *source_byte = (const uint8_t *)source;
        ptrdiff_t x;
        
        bool overlap = (dest_byte < (source_byte + in_bytes));
        
        ptrdiff_t bytes = ptrdiff_t( in_bytes );
        
        x = 0;
        if (overlap && source_byte < dest_byte ) {
            // we must copy in reverse
            ptrdiff_t dist = (source_byte + in_bytes) - dest_byte;
            
            if ( bytes > 64 && dist > 16 ) {
            
                // align dest to reverse 32 bit boundary
                for (x = 0; (x < bytes) && (((intptr_t)(dest_byte+bytes-1-x) & 0x03) != 0x03); ++x)
                    dest_byte[bytes-1-x] = source_byte[bytes-1-x];
                
                // copy 32 bit words
                for (; x < (bytes - 15); x += 16) {
                    *((uint32_t *)(dest_byte + bytes-1-(x+3))) = *((uint32_t *)(source_byte + bytes-1-(x+3)));
                    *((uint32_t *)(dest_byte + bytes-1-(x+4+3))) = *((uint32_t *)(source_byte + bytes-1-(x+4+3)));
                    *((uint32_t *)(dest_byte + bytes-1-(x+8+3))) = *((uint32_t *)(source_byte + bytes-1-(x+8+3)));
                    *((uint32_t *)(dest_byte + bytes-1-(x+12+3))) = *((uint32_t *)(source_byte + bytes-1-(x+12+3)));
                }
            
            }
            
            for (; x < bytes; ++x)
                dest_byte[bytes-1-x] = source_byte[bytes-1-x];
            
        } else {
            
            if ( bytes > 64 ) {
            
                // align to 32 bit boundary
                for (x = 0; x < bytes && ((intptr_t)(dest_byte+x) & 0x03); ++x)
                    dest_byte[x] = source_byte[x];
                
                // copy 32 bit words
                for (; x < (bytes - 15); x += 16) {
                    *((uint32_t *)(dest_byte + x + 0)) = *((uint32_t *)(source_byte + x + 0));
                    *((uint32_t *)(dest_byte + x + 4)) = *((uint32_t *)(source_byte + x + 4));
                    *((uint32_t *)(dest_byte + x + 8)) = *((uint32_t *)(source_byte + x + 8));
                    *((uint32_t *)(dest_byte + x + 12)) = *((uint32_t *)(source_byte + x + 12));
                }

            }
            
            // copy remaining bytes
            for ( ; x < bytes; ++x )
                dest_byte[x] = source_byte[x];
        }
        
        return dest;
    }

};

/******************************************************************************/

// loop unrolled, and copy 64 bit values whenever possible
struct forloop_unroll64_memmove {

    void * operator()( void *dest, const void *source, size_t in_bytes ) const {
        if (dest == source || in_bytes == 0)
            return dest;
        
        uint8_t *dest_byte = (uint8_t *)dest;
        const uint8_t *source_byte = (const uint8_t *)source;
        ptrdiff_t x;
        
        bool overlap = (dest_byte < (source_byte + in_bytes));
        
        ptrdiff_t bytes = ptrdiff_t( in_bytes );
        
        x = 0;
        if (overlap && source_byte < dest_byte ) {
            // we must copy in reverse
            ptrdiff_t dist = (source_byte + in_bytes) - dest_byte;
            
            if ( bytes > 64 && dist > 32 ) {
            
                // align dest to reverse 64 bit boundary
                for (x = 0; (x < bytes) && (((intptr_t)(dest_byte+bytes-1-x) & 0x07) != 0x07); ++x)
                    dest_byte[bytes-1-x] = source_byte[bytes-1-x];
                
                // copy 64 bit words
                for (; x < (bytes - 31); x += 32) {
                    *((uint64_t *)(dest_byte + bytes-1-(x+7))) = *((uint64_t *)(source_byte + bytes-1-(x+7)));
                    *((uint64_t *)(dest_byte + bytes-1-(x+8+7))) = *((uint64_t *)(source_byte + bytes-1-(x+8+7)));
                    *((uint64_t *)(dest_byte + bytes-1-(x+16+7))) = *((uint64_t *)(source_byte + bytes-1-(x+16+7)));
                    *((uint64_t *)(dest_byte + bytes-1-(x+24+7))) = *((uint64_t *)(source_byte + bytes-1-(x+24+7)));
                }

            }
            
            // copy remaining bytes
            for (; x < bytes; ++x)
                dest_byte[bytes-1-x] = source_byte[bytes-1-x];
            
        } else {
            
            if ( bytes > 64 ) {
            
                // align dest to 64 bit boundary
                for (x = 0; x < bytes && ((intptr_t)(dest_byte+x) & 0x07); ++x)
                    dest_byte[x] = source_byte[x];
                
                // copy 64 bit words
                for (; x < (bytes - 31); x += 32) {
                    *((uint64_t *)(dest_byte + x + 0)) = *((uint64_t *)(source_byte + x + 0));
                    *((uint64_t *)(dest_byte + x + 8)) = *((uint64_t *)(source_byte + x + 8));
                    *((uint64_t *)(dest_byte + x + 16)) = *((uint64_t *)(source_byte + x + 16));
                    *((uint64_t *)(dest_byte + x + 24)) = *((uint64_t *)(source_byte + x + 24));
                }
            }
            
            // copy remaining bytes
            for ( ; x < bytes; ++x )
                dest_byte[x] = source_byte[x];
        }
        
        return dest;
    }

};

/******************************************************************************/

// loop unrolled, and copy 64 bit values in 64 byte groups
struct forloop_unroll64_cacheline_memmove {

    void * operator()( void *dest, const void *source, size_t in_bytes ) const {
        if (dest == source || in_bytes == 0)
            return dest;
        
        uint8_t *dest_byte = (uint8_t *)dest;
        const uint8_t *source_byte = (const uint8_t *)source;
        ptrdiff_t x;
        
        ptrdiff_t dist = (source_byte + in_bytes) - dest_byte;
        
        ptrdiff_t bytes = ptrdiff_t( in_bytes );
        
        x = 0;
        if ((dist > 0) && (source_byte < dest_byte) ) {
            // we must copy in reverse
            uint8_t *dest_end = dest_byte + bytes-1;
            const uint8_t *src_end = source_byte + bytes-1;
            
            if ( bytes > 128 && dist > 64 ) {

                // align dest to reverse 64 bit boundary
                for (x = 0; (x < bytes) && (((intptr_t)(dest_byte+bytes-1-x) & 0x07) != 0x07); ++x)
                    dest_byte[bytes-1-x] = source_byte[bytes-1-x];
                
                // aligning to a cacheline boundary did not improve performance on Intel i7
                
                // copy 64 byte cachelines
                for (; x < (bytes - 63); x += 64) {
                    *((uint64_t *)(dest_end-(x+7))) = *((uint64_t *)(src_end-(x+0+7)));
                    *((uint64_t *)(dest_end-(x+8+7))) = *((uint64_t *)(src_end-(x+8+7)));
                    *((uint64_t *)(dest_end-(x+16+7))) = *((uint64_t *)(src_end-(x+16+7)));
                    *((uint64_t *)(dest_end-(x+24+7))) = *((uint64_t *)(src_end-(x+24+7)));
                    *((uint64_t *)(dest_end-(x+32+7))) = *((uint64_t *)(src_end-(x+32+7)));
                    *((uint64_t *)(dest_end-(x+40+7))) = *((uint64_t *)(src_end-(x+40+7)));
                    *((uint64_t *)(dest_end-(x+48+7))) = *((uint64_t *)(src_end-(x+48+7)));
                    *((uint64_t *)(dest_end-(x+56+7))) = *((uint64_t *)(src_end-(x+56+7)));
                }

            }
            
            // copy leftover bytes
            for (; x < bytes; ++x)
                dest_end[-x] = src_end[-x];
            
        } else {
            // we can copy forward
            
            if ( bytes > 128 ) {
                
                // align dest to 64 bit boundary
                for (x = 0; x < bytes && ((intptr_t)(dest_byte+x) & 0x07); ++x)
                    dest_byte[x] = source_byte[x];
                
                // aligning to a cacheline boundary did not improve performance on Intel i7
                
                // copy 64 bit words
                for (; x < (bytes - 63); x += 64) {
                    *((uint64_t *)(dest_byte + x + 0)) = *((uint64_t *)(source_byte + x + 0));
                    *((uint64_t *)(dest_byte + x + 8)) = *((uint64_t *)(source_byte + x + 8));
                    *((uint64_t *)(dest_byte + x + 16)) = *((uint64_t *)(source_byte + x + 16));
                    *((uint64_t *)(dest_byte + x + 24)) = *((uint64_t *)(source_byte + x + 24));
                    *((uint64_t *)(dest_byte + x + 32)) = *((uint64_t *)(source_byte + x + 32));
                    *((uint64_t *)(dest_byte + x + 40)) = *((uint64_t *)(source_byte + x + 40));
                    *((uint64_t *)(dest_byte + x + 48)) = *((uint64_t *)(source_byte + x + 48));
                    *((uint64_t *)(dest_byte + x + 56)) = *((uint64_t *)(source_byte + x + 56));
                }
            }
            
            // copy remaining bytes
            for ( ; x < bytes; ++x )
                dest_byte[x] = source_byte[x];
        }
        
        return dest;
    }

};

/******************************************************************************/
/******************************************************************************/

template <typename T, typename Move >
void test_memmove(T *dest, const T *source, int count, Move copier, const char *label) {
    int i;
    int bytes = count * sizeof(T);

    start_timer();

    for(i = 0; i < iterations; ++i) {
        copier( dest, source, bytes );
    }

    if (current_test < 0 || current_test > 100) {
        printf("**FATAL** Heap corrupted current_test is %d\n", current_test);
        exit(-4);
    }
    
    if (allocated_results < 0 || allocated_results > 100) {
        printf("**FATAL** Heap corrupted allocated_results is %d\n", allocated_results);
        exit(-2);
    }
    
    record_result( timer(), label );

    ptrdiff_t dist = dest - source;
    bool overlaps = (std::abs(dist) < count);
    
    // simple compare won't work when there is overlap between buffers - because we changed the src
    if ( !overlaps && memcmp(dest, source, bytes) != 0 )
        printf("test %s failed\n", label);

    // DEBUG
    //printf("src = 0x%8.8lX, dst = 0x%8.8lX, total %d, overlap %ld\n", (intptr_t)source, (intptr_t)dest, count, overlaps ? dist : 0 );
}

/******************************************************************************/

template <typename T, typename Move >
void test_memmove_sizes(T *dest, const T *source, int max_count, int overlap, Move copier, const std::string label) {
    int i, j;
    const int saved_iterations = iterations;
    
    printf("\ntest   description   absolute   operations\n");
    printf(  "number               time       per second\n\n");

    for( i = 4, j = 0; i <= max_count; i *= 2, ++j ) {
        
        int64_t temp1 = SIZE / i;
        int64_t temp2 = saved_iterations * temp1;

        if (temp2 > 0x70000000)
            temp2 = 0x70000000;
        if (temp2 < 4)
            temp2 = 4;
        
        iterations = temp2;
        const T *src = source;
        T *dst = dest;
        
        switch (overlap) {
            case -1:
                src = dest;
                dst = dest + i - (i/4);
                break;
            
            case 1:
                src = dest + i - (i/4);
                dst = dest;
                break;
            
            case -2:
                src = dest;
                dst = dest + (i/4);
                break;
            
            case 2:
                src = dest + (i/4);
                dst = dest;
                break;
            
            case 0:
            default:
                break;
        }
        
        test_memmove( dst, src, i, copier, label.c_str() );
        
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

uint8_t data8u[2*SIZE];
uint8_t data8u_source[SIZE];

/******************************************************************************/
/******************************************************************************/

template <typename T>
void TestSizes(T *dest, const T *src, int max_bytes, int overlap, std::string label) {

    test_memmove_sizes( dest, src, max_bytes, overlap, memmove, "memmove " + label );
    test_memmove_sizes( dest, src, max_bytes, overlap, std_move(), "std::move " + label );
    test_memmove_sizes( dest, src, max_bytes, overlap, iterator_memmove(), "iterator move " + label);
    test_memmove_sizes( dest, src, max_bytes, overlap, iterator_memmove2(), "iterator2 move " + label);
    test_memmove_sizes( dest, src, max_bytes, overlap, forloop_memmove(), "for loop move " + label);
    test_memmove_sizes( dest, src, max_bytes, overlap, forloop_memmove2(), "for loop2 move " + label);
    test_memmove_sizes( dest, src, max_bytes, overlap, forloop_unroll_memmove(), "for loop unroll move " + label);
    test_memmove_sizes( dest, src, max_bytes, overlap, forloop_unroll32_memmove(), "for loop unroll32 move " + label);
    test_memmove_sizes( dest, src, max_bytes, overlap, forloop_unroll64_memmove(), "for loop unroll64 move " + label);
    test_memmove_sizes( dest, src, max_bytes, overlap, forloop_unroll64_cacheline_memmove(), "for loop unroll64 cacheline move " + label);
}

/******************************************************************************/

int main(int argc, char** argv) {
    
    // output command for documentation:
    int i;
    for (i = 0; i < argc; ++i)
        printf("%s ", argv[i] );
    printf("\n");

    if (argc > 1) iterations = atoi(argv[1]);
    if (argc > 2) init_value = (int64_t) atoi(argv[2]);


    scrand( init_value );
    fill_random( data8u, data8u+2*SIZE );
    fill_random( data8u_source, data8u_source+SIZE );


    // src and dest don't overlap at all, can copy normally
    test_memmove( data8u, data8u_source, SIZE, memmove, "memmove separate");
    test_memmove( data8u, data8u_source, SIZE, std_move(), "std::move separate");
    test_memmove( data8u, data8u_source, SIZE, iterator_memmove(), "iterator move separate");
    test_memmove( data8u, data8u_source, SIZE, iterator_memmove2(), "iterator2 move separate");
    test_memmove( data8u, data8u_source, SIZE, forloop_memmove(), "for loop move separate");
    test_memmove( data8u, data8u_source, SIZE, forloop_memmove2(), "for loop2 move separate");
    test_memmove( data8u, data8u_source, SIZE, forloop_unroll_memmove(), "for loop unroll move separate");
    test_memmove( data8u, data8u_source, SIZE, forloop_unroll32_memmove(), "for loop unroll32 move separate");
    test_memmove( data8u, data8u_source, SIZE, forloop_unroll64_memmove(), "for loop unroll64 move separate");
    test_memmove( data8u, data8u_source, SIZE, forloop_unroll64_cacheline_memmove(), "for loop unroll64 cacheline move separate");

    summarize("memmove separate", SIZE, iterations, kDontShowGMeans, kDontShowPenalty );
    
    
    // src and dest don't overlap at all, can copy normally (unless bad logic is involved)
    test_memmove( data8u_source, data8u, SIZE, memmove, "memmove separate_reversed");
    test_memmove( data8u_source, data8u, SIZE, std_move(), "std::move separate_reversed");
    test_memmove( data8u_source, data8u, SIZE, iterator_memmove(), "iterator move separate_reversed");
    test_memmove( data8u_source, data8u, SIZE, iterator_memmove2(), "iterator2 move separate_reversed");
    test_memmove( data8u_source, data8u, SIZE, forloop_memmove(), "for loop move separate_reversed");
    test_memmove( data8u_source, data8u, SIZE, forloop_memmove2(), "for loop2 move separate_reversed");
    test_memmove( data8u_source, data8u, SIZE, forloop_unroll_memmove(), "for loop unroll move separate_reversed");
    test_memmove( data8u_source, data8u, SIZE, forloop_unroll32_memmove(), "for loop unroll32 move separate_reversed");
    test_memmove( data8u_source, data8u, SIZE, forloop_unroll64_memmove(), "for loop unroll64 move separate_reversed");
    test_memmove( data8u_source, data8u, SIZE, forloop_unroll64_cacheline_memmove(), "for loop unroll64 cacheline move separate_reversed");

    summarize("memmove separate_reversed", SIZE, iterations, kDontShowGMeans, kDontShowPenalty );


    // src and dest overlap near end, dest < src, can copy normally
    test_memmove( data8u, data8u+SIZE-(SIZE/4), SIZE, memmove, "memmove overlap forward");
    test_memmove( data8u, data8u+SIZE-(SIZE/4), SIZE, std_move(), "std::move overlap forward");
    test_memmove( data8u, data8u+SIZE-(SIZE/4), SIZE, iterator_memmove(), "iterator move overlap forward");
    test_memmove( data8u, data8u+SIZE-(SIZE/4), SIZE, iterator_memmove2(), "iterator2 move overlap forward");
    test_memmove( data8u, data8u+SIZE-(SIZE/4), SIZE, forloop_memmove(), "for loop move overlap forward");
    test_memmove( data8u, data8u+SIZE-(SIZE/4), SIZE, forloop_memmove2(), "for loop2 move overlap forward");
    test_memmove( data8u, data8u+SIZE-(SIZE/4), SIZE, forloop_unroll_memmove(), "for loop unroll move overlap forward");
    test_memmove( data8u, data8u+SIZE-(SIZE/4), SIZE, forloop_unroll32_memmove(), "for loop unroll32 move overlap forward");
    test_memmove( data8u, data8u+SIZE-(SIZE/4), SIZE, forloop_unroll64_memmove(), "for loop unroll64 move overlap forward");
    test_memmove( data8u, data8u+SIZE-(SIZE/4), SIZE, forloop_unroll64_cacheline_memmove(), "for loop unroll64 cacheline move overlap forward");

    summarize("memmove overlap forward", SIZE, iterations, kDontShowGMeans, kDontShowPenalty );


    // src and dest overlap , dest > src, must copy in reverse
    test_memmove( data8u+SIZE-(SIZE/4), data8u, SIZE, memmove, "memmove overlap reversed");
    test_memmove( data8u+SIZE-(SIZE/4), data8u, SIZE, std_move(), "std::move overlap reversed");
    test_memmove( data8u+SIZE-(SIZE/4), data8u, SIZE, iterator_memmove(), "iterator move overlap reversed");
    test_memmove( data8u+SIZE-(SIZE/4), data8u, SIZE, iterator_memmove2(), "iterator2 move overlap reversed");
    test_memmove( data8u+SIZE-(SIZE/4), data8u, SIZE, forloop_memmove(), "for loop move overlap reversed");
    test_memmove( data8u+SIZE-(SIZE/4), data8u, SIZE, forloop_memmove2(), "for loop2 move overlap reversed");
    test_memmove( data8u+SIZE-(SIZE/4), data8u, SIZE, forloop_unroll_memmove(), "for loop unroll move overlap reversed");
    test_memmove( data8u+SIZE-(SIZE/4), data8u, SIZE, forloop_unroll32_memmove(), "for loop unroll32 move overlap reversed");
    test_memmove( data8u+SIZE-(SIZE/4), data8u, SIZE, forloop_unroll64_memmove(), "for loop unroll64 move overlap reversed");
    test_memmove( data8u+SIZE-(SIZE/4), data8u, SIZE, forloop_unroll64_cacheline_memmove(), "for loop unroll64 cacheline move overlap reversed");
    
    summarize("memmove overlap reversed", SIZE, iterations, kDontShowGMeans, kDontShowPenalty );


    // complete overlap, copy not necessary
    test_memmove( data8u, data8u, SIZE, memmove, "memmove overlap inplace");
    test_memmove( data8u, data8u, SIZE, std_move(), "std::move overlap inplace");
    test_memmove( data8u, data8u, SIZE, iterator_memmove(), "iterator move overlap inplace");
    test_memmove( data8u, data8u, SIZE, iterator_memmove2(), "iterator2 move overlap inplace");
    test_memmove( data8u, data8u, SIZE, forloop_memmove(), "for loop move overlap inplace");
    test_memmove( data8u, data8u, SIZE, forloop_memmove2(), "for loop2 move overlap inplace");
    test_memmove( data8u, data8u, SIZE, forloop_unroll_memmove(), "for loop unroll move overlap inplace");
    test_memmove( data8u, data8u, SIZE, forloop_unroll32_memmove(), "for loop unroll32 move overlap inplace");
    test_memmove( data8u, data8u, SIZE, forloop_unroll64_memmove(), "for loop unroll64 move overlap inplace");
    test_memmove( data8u, data8u, SIZE, forloop_unroll64_cacheline_memmove(), "for loop unroll64 cacheline move overlap inplace");
    
    summarize("memmove overlap inplace", SIZE, iterations, kDontShowGMeans, kDontShowPenalty );


    // zero length, copy not necessary
    test_memmove( data8u, data8u_source, 0, memmove, "memmove overlap zero");
    test_memmove( data8u, data8u_source, 0, std_move(), "std::move overlap zero");
    test_memmove( data8u, data8u_source, 0, iterator_memmove(), "iterator move overlap zero");
    test_memmove( data8u, data8u_source, 0, iterator_memmove2(), "iterator2 move overlap zero");
    test_memmove( data8u, data8u_source, 0, forloop_memmove(), "for loop move overlap zero");
    test_memmove( data8u, data8u_source, 0, forloop_memmove2(), "for loop2 move overlap zero");
    test_memmove( data8u, data8u_source, 0, forloop_unroll_memmove(), "for loop unroll move overlap zero");
    test_memmove( data8u, data8u_source, 0, forloop_unroll32_memmove(), "for loop unroll32 move overlap zero");
    test_memmove( data8u, data8u_source, 0, forloop_unroll64_memmove(), "for loop unroll64 move overlap zero");
    test_memmove( data8u, data8u_source, 0, forloop_unroll64_cacheline_memmove(), "for loop unroll64 cacheline move overlap zero");
    
    summarize("memmove zero", SIZE, iterations, kDontShowGMeans, kDontShowPenalty );
    
    
    // test by different sizes
    TestSizes( data8u, data8u_source, SIZE, 0, "separate" );
    TestSizes( data8u_source, data8u, SIZE, 0, "separate_reversed" );
    TestSizes( data8u, data8u, SIZE, 1, "overlap forward" );
    TestSizes( data8u, data8u, SIZE, -1, "overlap reverse" );


    return 0;
}

// the end
/******************************************************************************/
/******************************************************************************/
