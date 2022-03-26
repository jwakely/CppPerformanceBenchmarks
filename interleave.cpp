/*
    Copyright 2008 Adobe Systems Incorporated
    Copyright 2019-2021 Chris Cox
    Distributed under the MIT License (see accompanying file LICENSE_1_0_0.txt
    or a copy at http://stlab.adobe.com/licenses.html )


Goal:  Test compiler optimizations related to interleaving multiple buffers.

Assumptions:

    1) The compiler will recognize and optimize data interleaving patterns.



NOTE - these patterns occur pretty often in graphics and signal processing
    AAAA,GGGG --> AGAGAGAG
    RRRR,GGGG,BBBB --> RGBRGBRGBRGB
    AAAA,RRRR,GGGG,BBBB --> ARGBARGBARGBARGB

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
#include "benchmark_algorithms.h"
#include "benchmark_typenames.h"

/******************************************************************************/

// this constant may need to be adjusted to give reasonable minimum times
// For best results, times should be about 1.0 sources for the minimum test run
int iterations = 320000;


// 8*80k to 8*640k of data, intended to be outside cache of most CPUs
const int SIZE = 80000;


// initial value for filling our arrays, may be changed from the command line
uint8_t init_value = 3;

/******************************************************************************/
/******************************************************************************/

template<typename T>
void verify_interleave_list( T *dest, const T *sourceList[], int length, int sourceCount, const std::string &label ) {
    int i, j;
    for ( i = 0; i < length; ++i ) {
        for ( j = 0; j < sourceCount; ++j ) {
            if ( *dest != *(sourceList[j] + i) ) {
                printf("test %s failed\n", label.c_str() );
                return;
            }
            dest++;
        }
    }
}

/******************************************************************************/
/******************************************************************************/

// strides are in units of T
template<typename T>
void copy_with_stride( T *dest, const T *source, size_t count, size_t dest_stride, size_t source_stride ) {
    for (size_t i = 0; i < count; ++i ) {
        *dest = *source;
        dest += dest_stride;
        source += source_stride;
    }
}

/******************************************************************************/
/******************************************************************************/

bool isLittleEndian()       // Difficult More Much Debugging and Reading Makes Endian Little
{
#if defined(__LITTLE_ENDIAN__) || defined( _LIBCPP_LITTLE_ENDIAN )
    return true;
#elif defined(__BIG_ENDIAN__) || defined(_LIBCPP_BIG_ENDIAN)
    return false;
#else
    uint32_t pattern = 0x01020304;
    return ( *((uint8_t*)(&pattern)) == 0x04);
#endif
}

/******************************************************************************/

bool isBigEndian()          // Big Endian is Much Easier to Read and Debug
{
#if defined(__BIG_ENDIAN__) || defined(_LIBCPP_BIG_ENDIAN)
    return true;
#elif defined(__LITTLE_ENDIAN__) || defined( _LIBCPP_LITTLE_ENDIAN )
    return false;
#else
    static uint32_t pattern = 0x01020304;
    return ( *((uint8_t*)(&pattern)) == 0x01);
#endif
}

/******************************************************************************/
/******************************************************************************/

// straightforward implementation, with array indices
// code/time should be identical to version 2
template<typename T>
void interleave2to2_version1( T *dest, const T *source1,
                        const T *source2, int count ) {
    for (int i = 0; i < count; ++i ) {
        dest[2*i+0] = source1[i];
        dest[2*i+1] = source2[i];
    }
}

/******************************************************************************/

// straightforward implementation, with incremented pointers
// code/time should be identical to version 1
template<typename T>
void interleave2to2_version2( T *dest, const T *source1,
                        const T *source2, int count ) {
    int i;
    for ( i = 0; i < count; ++i ) {
        *dest++ = *source1++;
        *dest++ = *source2++;
    }
}

/******************************************************************************/

// read and write one input at a time
// helps on some cache architectures, hurts on others
template<typename T>
void interleave2to2_version3( T *dest, const T *source1,
                        const T *source2, int count ) {
    copy_with_stride( dest+0, source1, count, 2, 1 );
    copy_with_stride( dest+1, source2, count, 2, 1 );
}

/******************************************************************************/

// read and write one inputs at a time, cacheblocked
// helps on some cache architectures, hurts on others
// code/time should be identical to version 5 and 6
template<typename T>
void interleave2to2_version4( T *dest, const T *source1,
                            const T *source2, int count ) {
    const int blockSize = 4096/(2*sizeof(T));
    
    for (int i = 0; i < count; i += blockSize) {
        int block = blockSize;
        if ( (i+block) > count)
            block = count - i;
        
        copy_with_stride( dest+0, source1, block, 2, 1 );
        copy_with_stride( dest+1, source2, block, 2, 1 );
        
        dest += 2*block;
        source1 += block;
        source2 += block;
    }
}

/******************************************************************************/

// read and write one inputs at a time, cacheblocked
// helps on some cache architectures, hurts on others
// code/time should be identical to version 4 and 6
template<typename T>
void interleave2to2_version5( T *dest, const T *source1,
                            const T *source2, int count ) {
    const int blockSize = 4096/(2*sizeof(T));
    
    for (int i = 0; i < count; i += blockSize) {
        int j;
        int block = blockSize;
        if ( (i+block) > count)
            block = count - i;
        
        for ( j = 0; j < block; ++j ) {
            dest[(2*j)+0] = source1[j];
        }
        
        for ( j = 0; j < block; ++j ) {
            dest[(2*j)+1] = source2[j];
        }
        
        dest += 2*block;
        source1 += block;
        source2 += block;
    }
}

/******************************************************************************/

// read and write one inputs at a time, cacheblocked
// helps on some cache architectures, hurts on others
// code/time should be identical to version 4 and 5
template<typename T>
void interleave2to2_version6( T *dest, const T *source1,
                            const T *source2, int count ) {
    const int blockSize = 4096/(2*sizeof(T));
    
    for (int i = 0; i < count; i += blockSize) {
        int j;
        int block = blockSize;
        if ( (i+block) > count)
            block = count - i;
        
        T *dstTmp = dest;
        for ( j = 0; j < block; ++j ) {
            *dstTmp = source1[j];
            dstTmp += 2;
        }
        
        dstTmp = dest + 1;
        for ( j = 0; j < block; ++j ) {
            *dstTmp = source2[j];
            dstTmp += 2;
        }
        
        dest += 2*block;
        source1 += block;
        source2 += block;
    }
}

/******************************************************************************/

// straightforward fallback implementation, with array indices
template<typename T>
void interleave2to2_version7( T *dest, const T *source1,
                        const T *source2, int count ) {
    for (int i = 0; i < count; ++i ) {
        dest[2*i+0] = source1[i];
        dest[2*i+1] = source2[i];
    }
}

// specialized implementation - read in a block, shift and mask to interleave, write in blocks
// This helps on some architectures, and hurts on others
template<>
void interleave2to2_version7( uint8_t *dest, const uint8_t *source1,
                        const uint8_t *source2, int count ) {
    int i = 0;
    const bool backwardsBytes = isLittleEndian();

    if (backwardsBytes) {
        for (; i < (count-3); i+=4 ) {
            uint32_t src1 = *((uint32_t*)(source1+i));
            uint32_t src2 = *((uint32_t*)(source2+i));

            uint32_t result1 = (src1 <<  0) & 0x000000ffUL;
            result1 |= (src2 <<  8)         & 0x0000ff00UL;
            result1 |= (src1 <<  8)         & 0x00ff0000UL;
            result1 |= (src2 << 16)         & 0xff000000UL;
            
            uint32_t result2 = (src1 >> 16) & 0x000000ffUL;
            result2 |= (src2 >>  8)         & 0x0000ff00UL;
            result2 |= (src1 >>  8)         & 0x00ff0000UL;
            result2 |= (src2 >>  0)         & 0xff000000UL;
            
            *((uint32_t*)(dest+2*i+0)) = result1;
            *((uint32_t*)(dest+2*i+4)) = result2;
        }
    } else {
        for (; i < (count-3); i+=4 ) {
            uint32_t src1 = *((uint32_t*)(source1+i));
            uint32_t src2 = *((uint32_t*)(source2+i));

            uint32_t result1 = (src1 >>  0) & 0xff000000UL;
            result1 |= (src2 >>  8)         & 0x00ff0000UL;
            result1 |= (src1 >>  8)         & 0x0000ff00UL;
            result1 |= (src2 >> 16)         & 0x000000ffUL;
            
            uint32_t result2 = (src1 << 16) & 0xff000000UL;
            result2 |= (src2 <<  8)         & 0x00ff0000UL;
            result2 |= (src1 <<  8)         & 0x0000ff00UL;
            result2 |= (src2 <<  0)         & 0x000000ffUL;
            
            *((uint32_t*)(dest+2*i+0)) = result1;
            *((uint32_t*)(dest+2*i+4)) = result2;
        }
    }
    
    // cleanup remaining bytes
    for (; i < count; ++i ) {
        dest[2*i+0] = source1[i];
        dest[2*i+1] = source2[i];
    }
}

// specialized implementation - read in a block, shift and mask to interleave, write in blocks
// This helps on some architectures, and hurts on others
template<>
void interleave2to2_version7( uint16_t *dest, const uint16_t *source1,
                        const uint16_t *source2, int count ) {
    int i = 0;
    const bool backwardsBytes = isLittleEndian();

    if (backwardsBytes) {
        for (; i < (count-1); i+=2 ) {
            uint32_t src1 = *((uint32_t*)(source1+i));
            uint32_t src2 = *((uint32_t*)(source2+i));

            uint64_t result1 = (src1 <<  0) & 0x0000ffffUL;
            result1 |= (src2 <<  16)        & 0xffff0000UL;
            
            uint64_t result2 = (src1 >> 16) & 0x0000ffffUL;
            result2 |= (src2 >>  0)         & 0xffff0000UL;

            *((uint32_t*)(dest+2*i+0)) = result1;
            *((uint32_t*)(dest+2*i+2)) = result2;
        }
    } else {
        for (; i < (count-1); i+=2 ) {
            uint32_t src1 = *((uint32_t*)(source1+i));
            uint32_t src2 = *((uint32_t*)(source2+i));
            
            uint64_t result1 = (src1 >>  0) & 0xffff0000UL;
            result1 |= (src2 >> 16)         & 0x0000ffffUL;
            
            uint64_t result2 = (src1 << 16) & 0xffff0000UL;
            result2 |= (src2 <<  0)         & 0x0000ffffUL;

            *((uint64_t*)(dest+2*i+0)) = result1;
            *((uint64_t*)(dest+2*i+2)) = result2;
        }
    }
    
    // cleanup remaining bytes
    for (; i < count; ++i ) {
        dest[2*i+0] = source1[i];
        dest[2*i+1] = source2[i];
    }
}

// only the bit pattern matters, cast to uint32_t
template<>
void interleave2to2_version7( float *dest, const float *source1,
                        const float *source2, int count ) {
    static_assert( sizeof(float) == sizeof(uint32_t), "float size unexpected" );
    interleave2to2_version7( (uint32_t *)(dest),
                               (const uint32_t *)(source1),
                               (const uint32_t *)(source2),
                               count );
}

// only the bit pattern matters, cast to uint64_t
template<>
void interleave2to2_version7( double *dest, const double *source1,
                        const double *source2, int count ) {
    static_assert( sizeof(double) == sizeof(uint64_t), "double size unexpected"  );
    interleave2to2_version7( (uint64_t *)(dest),
                               (const uint64_t *)(source1),
                               (const uint64_t *)(source2),
                               count );
}

/******************************************************************************/

// straightforward fallback implementation, with array indices
template<typename T>
void interleave2to2_version8( T *dest, const T *source1,
                        const T *source2, int count ) {
    for (int i = 0; i < count; ++i ) {
        dest[2*i+0] = source1[i];
        dest[2*i+1] = source2[i];
    }
}

// specialized implementation - read in a block, shift and mask to interleave, write in blocks
// This helps on some architectures, and hurts on others
template<>
void interleave2to2_version8( uint8_t *dest, const uint8_t *source1,
                        const uint8_t *source2, int count ) {
    int i = 0;
    const bool backwardsBytes = isLittleEndian();

    if (backwardsBytes) {
        for (; i < (count-7); i+=8 ) {
            uint64_t src1 = *((uint64_t*)(source1+i));
            uint64_t src2 = *((uint64_t*)(source2+i));

            uint64_t result1 = (src1 << 0) & 0xffULL;
            result1 |= (src2 <<  8) & 0xff00ULL;
            result1 |= (src1 <<  8) & 0xff0000ULL;
            result1 |= (src2 << 16) & 0xff000000ULL;
            result1 |= (src1 << 16) & 0xff00000000ULL;
            result1 |= (src2 << 24) & 0xff0000000000ULL;
            result1 |= (src1 << 24) & 0xff000000000000ULL;
            result1 |= (src2 << 32) & 0xff00000000000000ULL;
            
            uint64_t result2 = (src1 >> 32) & 0xffUL;
            result2 |= (src2 >> 24) & 0xff00ULL;
            result2 |= (src1 >> 24) & 0xff0000ULL;
            result2 |= (src2 >> 16) & 0xff000000ULL;
            result2 |= (src1 >> 16) & 0xff00000000ULL;
            result2 |= (src2 >>  8) & 0xff0000000000ULL;
            result2 |= (src1 >>  8) & 0xff000000000000ULL;
            result2 |= (src2 >>  0) & 0xff00000000000000ULL;
            
            *((uint64_t*)(dest+2*i+0)) = result1;
            *((uint64_t*)(dest+2*i+8)) = result2;
        }
    } else {
        for (; i < (count-7); i+=8 ) {
            uint64_t src1 = *((uint64_t*)(source1+i));
            uint64_t src2 = *((uint64_t*)(source2+i));

            uint64_t result1 = (src1 >> 0) & 0xff00000000000000ULL;
            result1 |= (src2 >>  8)        & 0x00ff000000000000ULL;
            result1 |= (src1 >>  8)        & 0x0000ff0000000000ULL;
            result1 |= (src2 >> 16)        & 0x000000ff00000000ULL;
            result1 |= (src1 >> 16)        & 0x00000000ff000000ULL;
            result1 |= (src2 >> 24)        & 0x0000000000ff0000ULL;
            result1 |= (src1 >> 24)        & 0x000000000000ff00ULL;
            result1 |= (src2 >> 32)        & 0x00000000000000ffULL;
            
            uint64_t result2 = (src1 << 32) & 0xff00000000000000ULL;
            result2 |= (src2 << 24)         & 0x00ff000000000000ULL;
            result2 |= (src1 << 24)         & 0x0000ff0000000000ULL;
            result2 |= (src2 << 16)         & 0x000000ff00000000ULL;
            result2 |= (src1 << 16)         & 0x00000000ff000000ULL;
            result2 |= (src2 <<  8)         & 0x0000000000ff0000ULL;
            result2 |= (src1 <<  8)         & 0x000000000000ff00ULL;
            result2 |= (src2 <<  0)         & 0x00000000000000ffULL;
            
            *((uint64_t*)(dest+2*i+0)) = result1;
            *((uint64_t*)(dest+2*i+8)) = result2;
        }
    }
    
    // cleanup remaining bytes
    for (; i < count; ++i ) {
        dest[2*i+0] = source1[i];
        dest[2*i+1] = source2[i];
    }
}

// specialized implementation - read in a block, shift and mask to interleave, write in blocks
// This helps on some architectures, and hurts on others
template<>
void interleave2to2_version8( uint16_t *dest, const uint16_t *source1,
                        const uint16_t *source2, int count ) {
    int i = 0;
    const bool backwardsBytes = isLittleEndian();

    if (backwardsBytes) {
        for (; i < (count-3); i+=4 ) {
            uint64_t src1 = *((uint64_t*)(source1+i));
            uint64_t src2 = *((uint64_t*)(source2+i));

            uint64_t result1 = (src1 <<  0) & 0x000000000000ffffULL;
            result1 |= (src2 <<  16)        & 0x00000000ffff0000ULL;
            result1 |= (src1 <<  16)        & 0x0000ffff00000000ULL;
            result1 |= (src2 <<  32)        & 0xffff000000000000ULL;
            
            uint64_t result2 = (src1 >> 32) & 0x000000000000ffffULL;
            result2 |= (src2 >> 16)         & 0x00000000ffff0000ULL;
            result2 |= (src1 >> 16)         & 0x0000ffff00000000ULL;
            result2 |= (src2 >>  0)         & 0xffff000000000000ULL;

            *((uint64_t*)(dest+2*i+0)) = result1;
            *((uint64_t*)(dest+2*i+4)) = result2;
        }
    } else {
        for (; i < (count-3); i+=4 ) {
            uint64_t src1 = *((uint64_t*)(source1+i));
            uint64_t src2 = *((uint64_t*)(source2+i));
            
            uint64_t result1 = (src1 >>  0) & 0xffff000000000000ULL;
            result1 |= (src2 >> 16)         & 0x0000ffff00000000ULL;
            result1 |= (src1 >> 16)         & 0x00000000ffff0000ULL;
            result1 |= (src2 >> 32)         & 0x000000000000ffffULL;
            
            uint64_t result2 = (src1 << 32) & 0xffff000000000000ULL;
            result2 |= (src2 << 16)         & 0x0000ffff00000000ULL;
            result2 |= (src1 << 16)         & 0x00000000ffff0000ULL;
            result2 |= (src2 <<  0)         & 0x000000000000ffffULL;

            *((uint64_t*)(dest+2*i+0)) = result1;
            *((uint64_t*)(dest+2*i+4)) = result2;
        }
    }
    
    // cleanup remaining bytes
    for (; i < count; ++i ) {
        dest[2*i+0] = source1[i];
        dest[2*i+1] = source2[i];
    }
}

// specialized implementation - read in a block, shift and mask to interleave, write in blocks
// This helps on some architectures, and hurts on others
template<>
void interleave2to2_version8( uint32_t *dest, const uint32_t *source1,
                        const uint32_t *source2, int count ) {
    int i = 0;
    const bool backwardsBytes = isLittleEndian();

    if (backwardsBytes) {
        for (; i < (count-3); i+=4 ) {
            uint64_t src1 = *((uint64_t*)(source1+i));
            uint64_t src2 = *((uint64_t*)(source2+i));
            uint64_t src3 = *((uint64_t*)(source1+i+2));
            uint64_t src4 = *((uint64_t*)(source2+i+2));

            uint64_t result1 = (src1 << 0)  & 0x00000000ffffffffULL;
            result1 |= (src2 << 32)         & 0xffffffff00000000ULL;
            
            uint64_t result2 = (src1 >> 32) & 0x00000000ffffffffULL;
            result2 |= (src2 >> 0)          & 0xffffffff00000000ULL;
            
            uint64_t result3 = (src3 << 0) & 0x00000000ffffffffULL;
            result3 |= (src4 << 32)         & 0xffffffff00000000ULL;
            
            uint64_t result4 = (src3 >> 32) & 0x00000000ffffffffULL;
            result4 |= (src4 >> 0)          & 0xffffffff00000000ULL;

            *((uint64_t*)(dest+2*i+0)) = result1;
            *((uint64_t*)(dest+2*i+2)) = result2;
            *((uint64_t*)(dest+2*i+4)) = result3;
            *((uint64_t*)(dest+2*i+6)) = result4;
        }
    } else {
        for (; i < (count-3); i+=4 ) {
            uint64_t src1 = *((uint64_t*)(source1+i));
            uint64_t src2 = *((uint64_t*)(source2+i));
            uint64_t src3 = *((uint64_t*)(source1+i+2));
            uint64_t src4 = *((uint64_t*)(source2+i+2));

            uint64_t result1 = (src1 >> 0)  & 0xffffffff00000000ULL;
            result1 |= (src2 >> 32)         & 0x00000000ffffffffULL;
            
            uint64_t result2 = (src1 << 32) & 0xffffffff00000000ULL;
            result2 |= (src2 << 0)          & 0x00000000ffffffffULL;
            
            uint64_t result3 = (src3 >> 0) & 0xffffffff00000000ULL;
            result3 |= (src4 >> 32)         & 0x00000000ffffffffULL;
            
            uint64_t result4 = (src3 << 32) & 0xffffffff00000000ULL;
            result4 |= (src4 << 0)          & 0x00000000ffffffffULL;

            *((uint64_t*)(dest+2*i+0)) = result1;
            *((uint64_t*)(dest+2*i+2)) = result2;
            *((uint64_t*)(dest+2*i+4)) = result3;
            *((uint64_t*)(dest+2*i+6)) = result4;
        }
    }
    
    // cleanup remaining bytes
    for (; i < count; ++i ) {
        dest[2*i+0] = source1[i];
        dest[2*i+1] = source2[i];
    }
}

// only the bit pattern matters, cast to uint32_t
template<>
void interleave2to2_version8( float *dest, const float *source1,
                        const float *source2, int count ) {
    static_assert( sizeof(float) == sizeof(uint32_t), "float size unexpected" );
    interleave2to2_version8( (uint32_t *)(dest),
                               (const uint32_t *)(source1),
                               (const uint32_t *)(source2),
                               count );
}

// only the bit pattern matters, cast to uint64_t
template<>
void interleave2to2_version8( double *dest, const double *source1,
                        const double *source2, int count ) {
    static_assert( sizeof(double) == sizeof(uint64_t), "double size unexpected"  );
    interleave2to2_version8( (uint64_t *)(dest),
                               (const uint64_t *)(source1),
                               (const uint64_t *)(source2),
                               count );
}

/******************************************************************************/
/******************************************************************************/

// straightforward implementation
template<typename T>
void interleave3to3_version1( T *dest, const T *source1, const T *source2,
                        const T *source3, int count ) {
    int i;
    for ( i = 0; i < count; ++i ) {
        dest[3*i+0] = source1[i];
        dest[3*i+1] = source2[i];
        dest[3*i+2] = source3[i];
    }
}

/******************************************************************************/

// read and write one input at a time
// helps on some cache architectures, hurts on others
template<typename T>
void interleave3to3_version2( T *dest, const T *source1, const T *source2,
                        const T *source3, int count ) {
    copy_with_stride( dest+0, source1, count, 3, 1 );
    copy_with_stride( dest+1, source2, count, 3, 1 );
    copy_with_stride( dest+2, source3, count, 3, 1 );
}

/******************************************************************************/

// read and write two inputs at a time
// helps on some cache architectures, hurts on others
template<typename T>
void interleave3to3_version3( T *dest, const T *source1, const T *source2,
                        const T *source3, int count ) {
    int i;
    for ( i = 0; i < count; ++i ) {
        dest[3*i+0] = source1[i];
        dest[3*i+1] = source2[i];
    }
    
    for ( i = 0; i < count; ++i ) {
        dest[3*i+2] = source3[i];
    }
}

/******************************************************************************/

// read and write two inputs at a time, cacheblocked
// helps on some cache architectures, hurts on others
template<typename T>
void interleave3to3_version4( T *dest, const T *source1, const T *source2,
                        const T *source3, int count ) {
    int i, j;
    const int blockSize = 4096/(3*sizeof(T));
    
    for ( i = 0; i < count; i += blockSize) {
        int block = blockSize;
        if ( (i+block) > count)
            block = count - i;
        
        for ( j = 0; j < block; ++j ) {
            dest[3*j+0] = source1[j];
            dest[3*j+1] = source2[j];
        }
        
        for ( j = 0; j < block; ++j ) {
            dest[3*j+2] = source3[j];
        }
        
        dest += 3*block;
        source1 += block;
        source2 += block;
        source3 += block;
    }
}

/******************************************************************************/

// read and write one inputs at a time, cacheblocked
// helps on some cache architectures, hurts on others
template<typename T>
void interleave3to3_version5( T *dest, const T *source1, const T *source2,
                        const T *source3, int count ) {
    int i, j;
    const int blockSize = 4096/(3*sizeof(T));
    
    for ( i = 0; i < count; i += blockSize) {
        int block = blockSize;
        if ( (i+block) > count)
            block = count - i;
        
        for ( j = 0; j < block; ++j ) {
            dest[3*j+0] = source1[j];
        }
        
        for ( j = 0; j < block; ++j ) {
            dest[3*j+1] = source2[j];
        }
        
        for ( j = 0; j < block; ++j ) {
            dest[3*j+2] = source3[j];
        }
        
        dest += 3*block;
        source1 += block;
        source2 += block;
        source3 += block;
    }
}

/******************************************************************************/
/******************************************************************************/

// straightforward implementation
template<typename T>
void interleave3to4_version1( T *dest, const T source1, const T *source2,
                        const T *source3, const T *source4, int count ) {
    int i;
    for ( i = 0; i < count; ++i ) {
        dest[4*i+0] = source1;
        dest[4*i+1] = source2[i];
        dest[4*i+2] = source3[i];
        dest[4*i+3] = source4[i];
    }
}

/******************************************************************************/

// read and write one input at a time
// helps on some cache architectures, hurts on others
template<typename T>
void interleave3to4_version2( T *dest, const T source1, const T *source2,
                        const T *source3, const T *source4, int count ) {
    T temp = source1;
    copy_with_stride( dest+0, &temp, count, 4, 0 );
    copy_with_stride( dest+1, source2, count, 4, 1 );
    copy_with_stride( dest+2, source3, count, 4, 1 );
    copy_with_stride( dest+3, source4, count, 4, 1 );
}

/******************************************************************************/

// read and write two inputs at a time
// helps on some cache architectures, hurts on others
template<typename T>
void interleave3to4_version3( T *dest, const T source1, const T *source2,
                        const T *source3, const T *source4, int count ) {
    int i;
    for ( i = 0; i < count; ++i ) {
        dest[4*i+0] = source1;
        dest[4*i+1] = source2[i];
    }
    
    for ( i = 0; i < count; ++i ) {
        dest[4*i+2] = source3[i];
        dest[4*i+3] = source4[i];
    }
}

/******************************************************************************/

// read and write one input at a time, cacheblocked
// helps on some cache architectures, hurts on others
template<typename T>
void interleave3to4_version4( T *dest, const T source1, const T *source2,
                        const T *source3, const T *source4, int count ) {
    int i;
    const int blockSize = 4096/(4*sizeof(T));
    T temp = source1;
    
    for ( i = 0; i < count; i += blockSize) {
        int block = blockSize;
        if ( (i+block) > count)
            block = count - i;
        
        copy_with_stride( dest+0, &temp, block, 4, 0 );
        copy_with_stride( dest+1, source2, block, 4, 1 );
        copy_with_stride( dest+2, source3, block, 4, 1 );
        copy_with_stride( dest+3, source4, block, 4, 1 );
        
        dest += 4*block;
        source2 += block;
        source3 += block;
        source4 += block;
    }
}

/******************************************************************************/

// read and write two inputs at a time, cacheblocked
// helps on some cache architectures, hurts on others
template<typename T>
void interleave3to4_version5( T *dest, const T source1, const T *source2,
                        const T *source3, const T *source4, int count ) {
    int i, j;
    const int blockSize = 4096/(4*sizeof(T));
    
    for ( i = 0; i < count; i += blockSize) {
        int block = blockSize;
        if ( (i+block) > count)
            block = count - i;
        
        for ( j = 0; j < block; ++j ) {
            dest[4*j+0] = source1;
            dest[4*j+1] = source2[j];
        }
        
        for ( j = 0; j < block; ++j ) {
            dest[4*j+2] = source3[j];
            dest[4*j+3] = source4[j];
        }
        
        dest += 4*block;
        source2 += block;
        source3 += block;
        source4 += block;
    }
}

/******************************************************************************/

// read and write one inputs at a time, cacheblocked
// helps on some cache architectures, hurts on others
template<typename T>
void interleave3to4_version6( T *dest, const T source1, const T *source2,
                        const T *source3, const T *source4, int count ) {
    int i, j;
    const int blockSize = 4096/(4*sizeof(T));
    
    for ( i = 0; i < count; i += blockSize) {
        int block = blockSize;
        if ( (i+block) > count)
            block = count - i;
        
        for ( j = 0; j < block; ++j ) {
            dest[4*j+0] = source1;
        }
        
        for ( j = 0; j < block; ++j ) {
            dest[4*j+1] = source2[j];
        }
        
        for ( j = 0; j < block; ++j ) {
            dest[4*j+2] = source3[j];
        }
        
        for ( j = 0; j < block; ++j ) {
            dest[4*j+3] = source4[j];
        }
        
        dest += 4*block;
        source2 += block;
        source3 += block;
        source4 += block;
    }
}

/******************************************************************************/
/******************************************************************************/

// straightforward implementation
template<typename T>
void interleave4to4_version1( T *dest, const T *source1, const T *source2,
                        const T *source3, const T *source4, int count ) {
    int i;
    for ( i = 0; i < count; ++i ) {
        dest[4*i+0] = source1[i];
        dest[4*i+1] = source2[i];
        dest[4*i+2] = source3[i];
        dest[4*i+3] = source4[i];
    }
}

/******************************************************************************/

// read and write one input at a time
// helps on some cache architectures, hurts on others
template<typename T>
void interleave4to4_version2( T *dest, const T *source1, const T *source2,
                        const T *source3, const T *source4, int count ) {
    copy_with_stride( dest+0, source1, count, 4, 1 );
    copy_with_stride( dest+1, source2, count, 4, 1 );
    copy_with_stride( dest+2, source3, count, 4, 1 );
    copy_with_stride( dest+3, source4, count, 4, 1 );
}

/******************************************************************************/

// read and write two inputs at a time
// helps on some cache architectures, hurts on others
template<typename T>
void interleave4to4_version3( T *dest, const T *source1, const T *source2,
                        const T *source3, const T *source4, int count ) {
    int i;
    for ( i = 0; i < count; ++i ) {
        dest[4*i+0] = source1[i];
        dest[4*i+1] = source2[i];
    }
    
    for ( i = 0; i < count; ++i ) {
        dest[4*i+2] = source3[i];
        dest[4*i+3] = source4[i];
    }
}

/******************************************************************************/

// read and write one input at a time, cacheblocked
// helps on some cache architectures, hurts on others
template<typename T>
void interleave4to4_version4( T *dest, const T *source1, const T *source2,
                        const T *source3, const T *source4, int count ) {
    int i;
    const int blockSize = 4096/(4*sizeof(T));
    
    for ( i = 0; i < count; i += blockSize) {
        int block = blockSize;
        if ( (i+block) > count)
            block = count - i;
        
        copy_with_stride( dest+0, source1, block, 4, 1 );
        copy_with_stride( dest+1, source2, block, 4, 1 );
        copy_with_stride( dest+2, source3, block, 4, 1 );
        copy_with_stride( dest+3, source4, block, 4, 1 );
        
        dest += 4*block;
        source1 += block;
        source2 += block;
        source3 += block;
        source4 += block;
    }
}

/******************************************************************************/

// read and write two inputs at a time, cacheblocked
// helps on some cache architectures, hurts on others
template<typename T>
void interleave4to4_version5( T *dest, const T *source1, const T *source2,
                        const T *source3, const T *source4, int count ) {
    int i, j;
    const int blockSize = 4096/(4*sizeof(T));
    
    for ( i = 0; i < count; i += blockSize) {
        int block = blockSize;
        if ( (i+block) > count)
            block = count - i;
        
        for ( j = 0; j < block; ++j ) {
            dest[4*j+0] = source1[j];
            dest[4*j+1] = source2[j];
        }
        
        for ( j = 0; j < block; ++j ) {
            dest[4*j+2] = source3[j];
            dest[4*j+3] = source4[j];
        }
        
        dest += 4*block;
        source1 += block;
        source2 += block;
        source3 += block;
        source4 += block;
    }
}

/******************************************************************************/

// read and write one input at a time, cacheblocked
// helps on some cache architectures, hurts on others
template<typename T>
void interleave4to4_version6( T *dest, const T *source1, const T *source2,
                        const T *source3, const T *source4, int count ) {
    int i, j;
    const int blockSize = 4096/(4*sizeof(T));
    
    for ( i = 0; i < count; i += blockSize) {
        int block = blockSize;
        if ( (i+block) > count)
            block = count - i;
        
        for ( j = 0; j < block; ++j ) {
            dest[4*j+0] = source1[j];
        }
        
        for ( j = 0; j < block; ++j ) {
            dest[4*j+1] = source2[j];
        }
        
        for ( j = 0; j < block; ++j ) {
            dest[4*j+2] = source3[j];
        }
        
        for ( j = 0; j < block; ++j ) {
            dest[4*j+3] = source4[j];
        }
        
        dest += 4*block;
        source1 += block;
        source2 += block;
        source3 += block;
        source4 += block;
    }
}

/******************************************************************************/
/******************************************************************************/

static std::deque<std::string> gLabels;


template <typename T, typename Move >
void test_interleave2to2(T *dest, const T *source1, const T *source2,
                    int count, Move interleave2to2, const std::string label) {
    int i;
    
    // make sure we are verifying this run correctly
    fill( dest, dest+2*count, T(0) );
    
    start_timer();

    for(i = 0; i < iterations; ++i) {
        interleave2to2( dest, source1, source2, count );
    }
    
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
    
    const T *sourceList[2];
    sourceList[0] = source1;
    sourceList[1] = source2;
    verify_interleave_list( dest, sourceList, count, 2, label );
}

/******************************************************************************/

template <typename T, typename Move >
void test_interleave3to3(T *dest, const T *source1, const T *source2,
                    const T *source3, int count,
                    Move interleave3to3, const std::string label) {
    int i;
    
    // make sure we are verifying this run correctly
    fill( dest, dest+3*count, T(0) );
    
    start_timer();

    for(i = 0; i < iterations; ++i) {
        interleave3to3( dest, source1, source2, source3, count );
    }
    
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
    
    const T *sourceList[3];
    sourceList[0] = source1;
    sourceList[1] = source2;
    sourceList[2] = source3;
    verify_interleave_list( dest, sourceList, count, 3, label );
}

/******************************************************************************/

template <typename T, typename Move >
void test_interleave3to4(T *dest, T *source1, const T *source2,
                    const T *source3, const T *source4, int count,
                    Move interleave3to4, const std::string label) {
    int i;
    
    // make sure we are verifying this run correctly
    fill( dest, dest+4*count, T(0) );
    
    // fill source1 with single value so we can verify the operation easily
    fill( source1, source1+count, source1[0] );
    
    start_timer();

    for(i = 0; i < iterations; ++i) {
        interleave3to4( dest, source1[0], source2, source3, source4, count );
    }
    
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );

    const T *sourceList[4];
    sourceList[0] = source1;
    sourceList[1] = source2;
    sourceList[2] = source3;
    sourceList[3] = source4;
    verify_interleave_list( dest, sourceList, count, 4, label );
}

/******************************************************************************/

template <typename T, typename Move >
void test_interleave4to4(T *dest, const T *source1, const T *source2,
                    const T *source3, const T *source4, int count,
                    Move interleave4to4, const std::string label) {
    int i;
    
    // make sure we are verifying this run correctly
    fill( dest, dest+4*count, T(0) );
    
    start_timer();

    for(i = 0; i < iterations; ++i) {
        interleave4to4( dest, source1, source2, source3, source4, count );
    }
    
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
    
    const T *sourceList[4];
    sourceList[0] = source1;
    sourceList[1] = source2;
    sourceList[2] = source3;
    sourceList[3] = source4;
    verify_interleave_list( dest, sourceList, count, 4, label );
}

/******************************************************************************/
/******************************************************************************/

template< typename T >
void TestOneType()
{
    std::string myTypeName( getTypeName<T>() );
    
    gLabels.clear();
    
    scrand( init_value+42 );
    
    auto base_iterations = iterations;

    // too much data for the stack
    std::unique_ptr<T> dA( new T[SIZE] );
    std::unique_ptr<T> dB( new T[SIZE] );
    std::unique_ptr<T> dC( new T[SIZE] );
    std::unique_ptr<T> dD( new T[SIZE] );
    std::unique_ptr<T> dE( new T[4*SIZE] );
    T *dataA = dA.get();
    T *dataB = dB.get();
    T *dataC = dC.get();
    T *dataD = dD.get();
    T *dataE = dE.get();

    fill_random( dataA, dataA+SIZE );
    fill_random( dataB, dataB+SIZE );
    fill_random( dataC, dataC+SIZE );
    fill_random( dataD, dataD+SIZE );
    fill( dataE, dataE+4*SIZE, T(0));

    iterations = base_iterations;
    test_interleave2to2( dataE, dataA, dataB, SIZE, interleave2to2_version1<T>, myTypeName + " interleave2to2_1");
    test_interleave2to2( dataE, dataA, dataB, SIZE, interleave2to2_version2<T>, myTypeName + " interleave2to2_2");
    test_interleave2to2( dataE, dataA, dataB, SIZE, interleave2to2_version3<T>, myTypeName + " interleave2to2_3");
    test_interleave2to2( dataE, dataA, dataB, SIZE, interleave2to2_version4<T>, myTypeName + " interleave2to2_4");
    test_interleave2to2( dataE, dataA, dataB, SIZE, interleave2to2_version5<T>, myTypeName + " interleave2to2_5");
    test_interleave2to2( dataE, dataA, dataB, SIZE, interleave2to2_version6<T>, myTypeName + " interleave2to2_6");
    test_interleave2to2( dataE, dataA, dataB, SIZE, interleave2to2_version7<T>, myTypeName + " interleave2to2_7");
    test_interleave2to2( dataE, dataA, dataB, SIZE, interleave2to2_version8<T>, myTypeName + " interleave2to2_8");
    
    std::string temp1( myTypeName + " interleave2to2" );
    summarize(temp1.c_str(), SIZE, iterations, kDontShowGMeans, kDontShowPenalty );


    iterations = (base_iterations * 2) / 3;
    test_interleave3to3( dataE, dataA, dataB, dataC, SIZE, interleave3to3_version1<T>, myTypeName + " interleave3to3_1");
    test_interleave3to3( dataE, dataA, dataB, dataC, SIZE, interleave3to3_version2<T>, myTypeName + " interleave3to3_2");
    test_interleave3to3( dataE, dataA, dataB, dataC, SIZE, interleave3to3_version3<T>, myTypeName + " interleave3to3_3");
    test_interleave3to3( dataE, dataA, dataB, dataC, SIZE, interleave3to3_version4<T>, myTypeName + " interleave3to3_4");
    test_interleave3to3( dataE, dataA, dataB, dataC, SIZE, interleave3to3_version5<T>, myTypeName + " interleave3to3_5");
    
    std::string temp2( myTypeName + " interleave3to3" );
    summarize(temp2.c_str(), SIZE, iterations, kDontShowGMeans, kDontShowPenalty );


    iterations = base_iterations / 2;
    test_interleave3to4( dataE, dataA, dataB, dataC, dataD, SIZE, interleave3to4_version1<T>, myTypeName + " interleave3to4_1");
    test_interleave3to4( dataE, dataA, dataB, dataC, dataD, SIZE, interleave3to4_version2<T>, myTypeName + " interleave3to4_2");
    test_interleave3to4( dataE, dataA, dataB, dataC, dataD, SIZE, interleave3to4_version3<T>, myTypeName + " interleave3to4_3");
    test_interleave3to4( dataE, dataA, dataB, dataC, dataD, SIZE, interleave3to4_version4<T>, myTypeName + " interleave3to4_4");
    test_interleave3to4( dataE, dataA, dataB, dataC, dataD, SIZE, interleave3to4_version5<T>, myTypeName + " interleave3to4_5");
    test_interleave3to4( dataE, dataA, dataB, dataC, dataD, SIZE, interleave3to4_version6<T>, myTypeName + " interleave3to4_6");
    
    std::string temp3( myTypeName + " interleave3to4" );
    summarize(temp3.c_str(), SIZE, iterations, kDontShowGMeans, kDontShowPenalty );


    iterations = base_iterations / 2;
    test_interleave4to4( dataE, dataA, dataB, dataC, dataD, SIZE, interleave4to4_version1<T>, myTypeName + " interleave4to4_1");
    test_interleave4to4( dataE, dataA, dataB, dataC, dataD, SIZE, interleave4to4_version2<T>, myTypeName + " interleave4to4_2");
    test_interleave4to4( dataE, dataA, dataB, dataC, dataD, SIZE, interleave4to4_version3<T>, myTypeName + " interleave4to4_3");
    test_interleave4to4( dataE, dataA, dataB, dataC, dataD, SIZE, interleave4to4_version4<T>, myTypeName + " interleave4to4_4");
    test_interleave4to4( dataE, dataA, dataB, dataC, dataD, SIZE, interleave4to4_version5<T>, myTypeName + " interleave4to4_5");
    test_interleave4to4( dataE, dataA, dataB, dataC, dataD, SIZE, interleave4to4_version6<T>, myTypeName + " interleave4to4_6");
    
    std::string temp4( myTypeName + " interleave4to4" );
    summarize(temp4.c_str(), SIZE, iterations, kDontShowGMeans, kDontShowPenalty );


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
    if (argc > 2) init_value = atoi(argv[2]);

    TestOneType<uint8_t>();
    iterations /= 2;
    TestOneType<uint16_t>();
    iterations /= 2;
    TestOneType<uint32_t>();
    iterations /= 2;
    TestOneType<uint64_t>();
    
    iterations *= 2;
    TestOneType<float>();
    iterations /= 2;
    TestOneType<double>();
    TestOneType<long double>();
    

    return 0;
}

// the end
/******************************************************************************/
/******************************************************************************/
