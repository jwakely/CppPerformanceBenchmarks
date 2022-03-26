/*
    Copyright 2008 Adobe Systems Incorporated
    Copyright 2019-2022 Chris Cox
    Distributed under the MIT License (see accompanying file LICENSE_1_0_0.txt
    or a copy at http://stlab.adobe.com/licenses.html )


Goal:  Test compiler optimizations related to deinterleaving buffers

Assumptions:

    1) The compiler will recognize and optimize data deinterleaving patterns.



NOTE - these patterns occur pretty often in graphics and signal processing
    AGAGAGAG  -->  AAAA,GGGG
    RGBRGBRGBRGB  -->  RRRR,GGGG,BBBB
    ARGBARGBARGBARGB  -->  AAAA,RRRR,GGGG,BBBB

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
int init_value = 2;

/******************************************************************************/
/******************************************************************************/

template<typename T>
void verify_deinterleave_list( const T *source, T *destList[], int length, int sourceCount, const std::string &label ) {
    int i, j;
    for ( i = 0; i < length; ++i ) {
        for ( j = 0; j < sourceCount; ++j ) {
            if (destList[j] != NULL) {
                if ( *source != *(destList[j] + i) ) {
                    printf("test %s failed\n", label.c_str());
                    return;
                }
            }
            source++;
        }
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

// straightforward implementation, with array indices
template<typename T>
void deinterleave2to2_version1( const T *source, T *dest1,
                                T *dest2, int count ) {
    for (int i = 0; i < count; ++i ) {
        dest1[i] = source[2*i+0];
        dest2[i] = source[2*i+1];
    }
}

/******************************************************************************/

// straightforward implementation, with incremented pointers
template<typename T>
void deinterleave2to2_version2( const T *source, T *dest1,
                                T *dest2, int count ) {
    for (int i = 0; i < count; ++i ) {
        *dest1++ = *source++;
        *dest2++ = *source++;
    }
}

/******************************************************************************/

// read and write one input at a time
// helps on some cache architectures, hurts on others
template<typename T>
void deinterleave2to2_version3( const T *source, T *dest1,
                                T *dest2, int count ) {
    copy_with_stride( dest1, source+0, count, 1, 2 );
    copy_with_stride( dest2, source+1, count, 1, 2 );
}

/******************************************************************************/

// read and write one input at a time, cacheblocked
// helps on some cache architectures, hurts on others
// code/time should be identical to version 5 and 6
template<typename T>
void deinterleave2to2_version4( const T *source, T *dest1,
                                T *dest2, int count ) {
    const int blockSize = 4096/(2*sizeof(T));
    
    for (int i = 0; i < count; i += blockSize) {
        int block = blockSize;
        if ( (i+block) > count)
            block = count - i;
        
        copy_with_stride( dest1, source+0, block, 1, 2 );
        copy_with_stride( dest2, source+1, block, 1, 2 );
        
        dest1 += block;
        dest2 += block;
        source += 2*block;
    }
}

/******************************************************************************/

// read and write one inputs at a time, cacheblocked
// helps on some cache architectures, hurts on others
// code/time should be identical to version 4 and 6
template<typename T>
void deinterleave2to2_version5( const T *source, T *dest1,
                                T *dest2, int count ) {
    const int blockSize = 4096/(2*sizeof(T));
    
    for (int i = 0; i < count; i += blockSize) {
        int j;
        int block = blockSize;
        if ( (i+block) > count)
            block = count - i;
        
        for ( j = 0; j < block; ++j ) {
            dest1[j] = source[2*j+0];
        }
        
        for ( j = 0; j < block; ++j ) {
            dest2[j] = source[2*j+1];
        }
        
        dest1 += block;
        dest2 += block;
        source += 2*block;
    }
}

/******************************************************************************/

// read and write one inputs at a time, cacheblocked
// helps on some cache architectures, hurts on others
// code/time should be identical to version 4 and 5
template<typename T>
void deinterleave2to2_version6( const T *source, T *dest1,
                                T *dest2, int count ) {
    const int blockSize = 4096/(2*sizeof(T));
    
    for (int i = 0; i < count; i += blockSize) {
        int j;
        int block = blockSize;
        if ( (i+block) > count)
            block = count - i;
        
        T *dstTmp = dest1;
        for ( j = 0; j < block; ++j ) {
            *dstTmp = source[2*j+0];
            dstTmp ++;
        }
        
        dstTmp = dest2;
        for ( j = 0; j < block; ++j ) {
            *dstTmp = source[2*j+1];
            dstTmp ++;
        }
        
        dest1 += block;
        dest2 += block;
        source += 2*block;
    }
}

/******************************************************************************/

// baseline for unspecialized types
template<typename T>
void deinterleave2to2_version7( const T *source, T *dest1,
                                T *dest2, int count ) {
    for (int i = 0; i < count; ++i ) {
        dest1[i] = source[2*i+0];
        dest2[i] = source[2*i+1];
    }
}

// unroll, read and write 32 bits at a time
// this is likely to confuse auto-vectorization, but can work well on scalar CPUs with fast shifters
template<>
void deinterleave2to2_version7( const uint8_t *source, uint8_t *dest1,
                                uint8_t *dest2, int count ) {
    int i = 0;
    const bool backwardsBytes = isLittleEndian();

    if (backwardsBytes) {
        for (; i < (count-3); i+=4 ) {
            uint32_t src1 = *((uint32_t*)(source+2*i));
            uint32_t src2 = *((uint32_t*)(source+2*i+4));

            uint32_t result1 = (src1 >>  0) & 0x000000ffUL;
            uint32_t result2 = (src1 >>  8) & 0x000000ffUL;
            result1 |= (src1 >>  8)         & 0x0000ff00UL;
            result2 |= (src1 >> 16)         & 0x0000ff00UL;
            
            uint32_t result3 = (src2 >>  0) & 0x000000ffUL;
            uint32_t result4 = (src2 >>  8) & 0x000000ffUL;
            result3 |= (src2 >>  8)         & 0x0000ff00UL;
            result4 |= (src2 >> 16)         & 0x0000ff00UL;
            
            result1 |= (result3 << 16);
            result2 |= (result4 << 16);
            
            *((uint32_t*)(dest1+i)) = result1;
            *((uint32_t*)(dest2+i)) = result2;
        }
    } else {
        for (; i < (count-3); i+=4 ) {
            uint32_t src1 = *((uint32_t*)(source+2*i));
            uint32_t src2 = *((uint32_t*)(source+2*i+4));

            uint32_t result1 = (src1 <<  0) & 0xff000000UL;
            uint32_t result2 = (src1 <<  8) & 0xff000000UL;
            result1 |= (src1 <<  8)         & 0x00ff0000UL;
            result2 |= (src1 << 16)         & 0x00ff0000UL;
            
            uint32_t result3 = (src2 >>  0) & 0xff000000UL;
            uint32_t result4 = (src2 <<  8) & 0xff000000UL;
            result3 |= (src2 <<  8)         & 0x00ff0000UL;
            result4 |= (src2 << 16)         & 0x00ff0000UL;
            
            result1 |= (result3 >> 16);
            result2 |= (result4 >> 16);
            
            *((uint32_t*)(dest1+i)) = result1;
            *((uint32_t*)(dest2+i)) = result2;
        }
    }
    
    // cleanup remaining bytes
    for (; i < count; ++i ) {
        dest1[i] = source[2*i+0];
        dest2[i] = source[2*i+1];
    }
}

// unroll, read and write 32 bits at a time
// this is likely to confuse auto-vectorization, but can work well on scalar CPUs with fast shifters
template<>
void deinterleave2to2_version7( const uint16_t *source, uint16_t *dest1,
                                uint16_t *dest2, int count ) {
    int i = 0;
    const bool backwardsBytes = isLittleEndian();

    if (backwardsBytes) {
        for (; i < (count-1); i+=2 ) {
            uint32_t src1 = *((uint32_t*)(source+2*i));
            uint32_t src2 = *((uint32_t*)(source+2*i+2));

            uint32_t result1 = (src1 >>  0) & 0x0000ffffUL;
            uint32_t result2 = (src1 >> 16) & 0x0000ffffUL;
            
            uint32_t result3 = (src2 << 16) & 0xffff0000UL;
            uint32_t result4 = (src2 >>  0) & 0xffff0000UL;

            result1 |= result3;
            result2 |= result4;
            
            *((uint32_t*)(dest1+i)) = result1;
            *((uint32_t*)(dest2+i)) = result2;
            
        }
    } else {
        for (; i < (count-1); i+=2 ) {
            uint32_t src1 = *((uint32_t*)(source+2*i));
            uint32_t src2 = *((uint32_t*)(source+2*i+2));

            uint32_t result1 = (src1 <<  0) & 0xffff0000UL;
            uint32_t result2 = (src1 << 16) & 0xffff0000UL;
            
            uint32_t result3 = (src2 >> 16) & 0x0000ffffUL;
            uint32_t result4 = (src2 >>  0) & 0x0000ffffUL;
            
            result1 |= result3;
            result2 |= result4;
            
            *((uint32_t*)(dest1+i)) = result1;
            *((uint32_t*)(dest2+i)) = result2;
        }
    }
    
    // cleanup remaining words
    for (; i < count; ++i ) {
        dest1[i] = source[2*i+0];
        dest2[i] = source[2*i+1];
    }
}

// only the bit pattern matters, cast to uint32_t
template<>
void deinterleave2to2_version7( const float *source, float *dest1,
                                float *dest2, int count ) {
    static_assert( sizeof(float) == sizeof(uint32_t), "float size unexpected" );
    deinterleave2to2_version7( (const uint32_t *)(source),
                               (uint32_t *)(dest1),
                               (uint32_t *)(dest2),
                               count );
}

// only the bit pattern matters, cast to uint64_t
template<>
void deinterleave2to2_version7( const double *source, double *dest1,
                                double *dest2, int count ) {
    static_assert( sizeof(double) == sizeof(uint64_t), "double size unexpected"  );
    deinterleave2to2_version7( (const uint64_t *)(source),
                               (uint64_t *)(dest1),
                               (uint64_t *)(dest2),
                               count );
}

/******************************************************************************/

// baseline for unspecialized types
template<typename T>
void deinterleave2to2_version8( const T *source, T *dest1,
                                T *dest2, int count ) {
    for (int i = 0; i < count; ++i ) {
        dest1[i] = source[2*i+0];
        dest2[i] = source[2*i+1];
    }
}

// unroll, read and write 64 bits at a time
// this is likely to confuse auto-vectorization, but can work well on scalar CPUs with fast shifters
template<>
void deinterleave2to2_version8( const uint8_t *source, uint8_t *dest1,
                                uint8_t *dest2, int count ) {
    int i = 0;
    const bool backwardsBytes = isLittleEndian();

    if (backwardsBytes) {
        for (; i < (count-7); i+=8 ) {
            uint64_t src1 = *((uint64_t*)(source+2*i));
            uint64_t src2 = *((uint64_t*)(source+2*i+8));

            uint64_t result1 = (src1 >>  0) & 0x00000000000000ffULL;
            uint64_t result2 = (src1 >>  8) & 0x00000000000000ffULL;
            result1 |= (src1 >>  8)         & 0x000000000000ff00ULL;
            result2 |= (src1 >> 16)         & 0x000000000000ff00ULL;
            result1 |= (src1 >> 16)         & 0x0000000000ff0000ULL;
            result2 |= (src1 >> 24)         & 0x0000000000ff0000ULL;
            result1 |= (src1 >> 24)         & 0x00000000ff000000ULL;
            result2 |= (src1 >> 32)         & 0x00000000ff000000ULL;
            
            uint64_t result3 = (src2 >>  0) & 0x00000000000000ffULL;
            uint64_t result4 = (src2 >>  8) & 0x00000000000000ffULL;
            result3 |= (src2 >>  8)         & 0x000000000000ff00ULL;
            result4 |= (src2 >> 16)         & 0x000000000000ff00ULL;
            result3 |= (src2 >> 16)         & 0x0000000000ff0000ULL;
            result4 |= (src2 >> 24)         & 0x0000000000ff0000ULL;
            result3 |= (src2 >> 24)         & 0x00000000ff000000ULL;
            result4 |= (src2 >> 32)         & 0x00000000ff000000ULL;
            
            result1 |= (result3 << 32);
            result2 |= (result4 << 32);
            
            *((uint64_t*)(dest1+i)) = result1;
            *((uint64_t*)(dest2+i)) = result2;
        }
    } else {
        for (; i < (count-7); i+=8 ) {
            uint64_t src1 = *((uint64_t*)(source+2*i));
            uint64_t src2 = *((uint64_t*)(source+2*i+8));

            uint64_t result1 = (src1 <<  0) & 0xff00000000000000ULL;
            uint64_t result2 = (src1 <<  8) & 0xff00000000000000ULL;
            result1 |= (src1 <<  8)         & 0x00ff000000000000ULL;
            result2 |= (src1 << 16)         & 0x00ff000000000000ULL;
            result1 |= (src1 << 16)         & 0x0000ff0000000000ULL;
            result2 |= (src1 << 24)         & 0x0000ff0000000000ULL;
            result1 |= (src1 << 24)         & 0x000000ff00000000ULL;
            result2 |= (src1 << 32)         & 0x000000ff00000000ULL;
            
            uint64_t result3 = (src2 >>  0) & 0xff00000000000000ULL;
            uint64_t result4 = (src2 <<  8) & 0xff00000000000000ULL;
            result3 |= (src2 <<  8)         & 0x00ff000000000000ULL;
            result4 |= (src2 << 16)         & 0x00ff000000000000ULL;
            result3 |= (src2 << 16)         & 0x0000ff0000000000ULL;
            result4 |= (src2 << 24)         & 0x0000ff0000000000ULL;
            result3 |= (src2 << 24)         & 0x000000ff00000000ULL;
            result4 |= (src2 << 32)         & 0x000000ff00000000ULL;
            
            result1 |= (result3 >> 32);
            result2 |= (result4 >> 32);
            
            *((uint64_t*)(dest1+i)) = result1;
            *((uint64_t*)(dest2+i)) = result2;
        }
    }
    
    // cleanup remaining bytes
    for (; i < count; ++i ) {
        dest1[i] = source[2*i+0];
        dest2[i] = source[2*i+1];
    }
}

// unroll, read and write 64 bits at a time
// this is likely to confuse auto-vectorization, but can work well on scalar CPUs with fast shifters
template<>
void deinterleave2to2_version8( const uint16_t *source, uint16_t *dest1,
                                uint16_t *dest2, int count ) {
    int i = 0;
    const bool backwardsBytes = isLittleEndian();

    if (backwardsBytes) {
        for (; i < (count-3); i+=4 ) {
            uint64_t src1 = *((uint64_t*)(source+2*i));
            uint64_t src2 = *((uint64_t*)(source+2*i+4));

            uint64_t result1 = (src1 >>  0) & 0x0000ffffUL;
            uint64_t result2 = (src1 >> 16) & 0x0000ffffUL;
            result1 |= (src1 >> 16)         & 0xffff0000UL;
            result2 |= (src1 >> 32)         & 0xffff0000UL;
            
            uint64_t result3 = (src2 >>  0) & 0x0000ffffUL;
            uint64_t result4 = (src2 >> 16) & 0x0000ffffUL;
            result3 |= (src2 >> 16)         & 0xffff0000UL;
            result4 |= (src2 >> 32)         & 0xffff0000UL;

            result1 |= (result3 << 32);
            result2 |= (result4 << 32);
            *((uint64_t*)(dest1+i)) = result1;
            *((uint64_t*)(dest2+i)) = result2;
            
        }
    } else {
        for (; i < (count-3); i+=4 ) {
            uint64_t src1 = *((uint64_t*)(source+2*i));
            uint64_t src2 = *((uint64_t*)(source+2*i+4));

            uint64_t result1 = (src1 <<  0) & 0xffff000000000000ULL;
            uint64_t result2 = (src1 << 16) & 0xffff000000000000ULL;
            result1 |= (src1 << 16)         & 0x0000ffff00000000ULL;
            result2 |= (src1 << 32)         & 0x0000ffff00000000ULL;
            
            uint64_t result3 = (src2 <<  0) & 0xffff000000000000ULL;
            uint64_t result4 = (src2 << 16) & 0xffff000000000000ULL;
            result3 |= (src2 << 16)         & 0x0000ffff00000000ULL;
            result4 |= (src2 << 32)         & 0x0000ffff00000000ULL;
            
            result1 |= (result3 >> 32);
            result2 |= (result4 >> 32);
            
            *((uint64_t*)(dest1+i)) = result1;
            *((uint64_t*)(dest2+i)) = result2;
        }
    }
    
    // cleanup remaining words
    for (; i < count; ++i ) {
        dest1[i] = source[2*i+0];
        dest2[i] = source[2*i+1];
    }
}

// unroll, read and write 64 bits at a time
// this is likely to confuse auto-vectorization, but can work well on scalar CPUs with fast shifters
template<>
void deinterleave2to2_version8( const uint32_t *source, uint32_t *dest1,
                                uint32_t *dest2, int count ) {
    int i = 0;
    const bool backwardsBytes = isLittleEndian();

    if (backwardsBytes) {
        for (; i < (count-3); i+=4 ) {
            uint64_t src1 = *((uint64_t*)(source+2*i));
            uint64_t src2 = *((uint64_t*)(source+2*i+2));
            uint64_t src3 = *((uint64_t*)(source+2*i+4));
            uint64_t src4 = *((uint64_t*)(source+2*i+6));

            uint64_t result1 = (src1 >>  0) & 0x00000000ffffffffULL;
            uint64_t result2 = (src1 >> 32) & 0x00000000ffffffffULL;
            result1 |= (src2 << 32)         & 0xffffffff00000000ULL;
            result2 |= (src2 <<  0)         & 0xffffffff00000000ULL;
            
            uint64_t result3 = (src3 >>  0) & 0x00000000ffffffffULL;
            uint64_t result4 = (src3 >> 32) & 0x00000000ffffffffULL;
            result3 |= (src4 << 32)         & 0xffffffff00000000ULL;
            result4 |= (src4 <<  0)         & 0xffffffff00000000ULL;
            
            *((uint64_t*)(dest1+i+0)) = result1;
            *((uint64_t*)(dest2+i+0)) = result2;
            *((uint64_t*)(dest1+i+2)) = result3;
            *((uint64_t*)(dest2+i+2)) = result4;
        }
    } else {
        for (; i < (count-3); i+=4 ) {
            uint64_t src1 = *((uint64_t*)(source+2*i));
            uint64_t src2 = *((uint64_t*)(source+2*i+2));
            uint64_t src3 = *((uint64_t*)(source+2*i+4));
            uint64_t src4 = *((uint64_t*)(source+2*i+6));

            uint64_t result1 = (src1 <<  0) & 0xffffffff00000000ULL;
            uint64_t result2 = (src1 << 32) & 0xffffffff00000000ULL;
            result1 |= (src2 >> 32)         & 0x00000000ffffffffULL;
            result2 |= (src2 <<  0)         & 0x00000000ffffffffULL;
            
            uint64_t result3 = (src3 <<  0) & 0xffffffff00000000ULL;
            uint64_t result4 = (src3 << 32) & 0xffffffff00000000ULL;
            result3 |= (src4 >> 32)         & 0x00000000ffffffffULL;
            result4 |= (src4 <<  0)         & 0x00000000ffffffffULL;
            
            *((uint64_t*)(dest1+i+0)) = result1;
            *((uint64_t*)(dest2+i+0)) = result2;
            *((uint64_t*)(dest1+i+2)) = result3;
            *((uint64_t*)(dest2+i+2)) = result4;
        }
    }
    
    // cleanup remaining words
    for (; i < count; ++i ) {
        dest1[i] = source[2*i+0];
        dest2[i] = source[2*i+1];
    }
}

// only the bit pattern matters, cast to uint32_t
template<>
void deinterleave2to2_version8( const float *source, float *dest1,
                                float *dest2, int count ) {
    static_assert( sizeof(float) == sizeof(uint32_t), "float size unexpected" );
    deinterleave2to2_version7( (const uint32_t *)(source),
                               (uint32_t *)(dest1),
                               (uint32_t *)(dest2),
                               count );
}

// only the bit pattern matters, cast to uint64_t
template<>
void deinterleave2to2_version8( const double *source, double *dest1,
                                double *dest2, int count ) {
    static_assert( sizeof(double) == sizeof(uint64_t), "double size unexpected"  );
    deinterleave2to2_version7( (const uint64_t *)(source),
                               (uint64_t *)(dest1),
                               (uint64_t *)(dest2),
                               count );
}

/******************************************************************************/
/******************************************************************************/

// straightforward implementation
template<typename T>
void deinterleave3to3_version1( const T *source, T *dest1, T *dest2,
                                T *dest3, int count ) {
    int i;
    for ( i = 0; i < count; ++i ) {
        dest1[i] = source[3*i+0];
        dest2[i] = source[3*i+1];
        dest3[i] = source[3*i+2];
    }
}

/******************************************************************************/

// read and write one input at a time
// helps on some cache architectures, hurts on others
template<typename T>
void deinterleave3to3_version2( const T *source, T *dest1, T *dest2,
                                T *dest3, int count ) {
    copy_with_stride( dest1, source+0, count, 1, 3 );
    copy_with_stride( dest2, source+1, count, 1, 3 );
    copy_with_stride( dest3, source+2, count, 1, 3 );
}

/******************************************************************************/

// read and write two inputs at a time
// helps on some cache architectures
template<typename T>
void deinterleave3to3_version3( const T *source, T *dest1, T *dest2,
                                T *dest3, int count ) {
    int i;
    for ( i = 0; i < count; ++i ) {
        dest1[i] = source[3*i+0];
        dest2[i] = source[3*i+1];
    }
    
    for ( i = 0; i < count; ++i ) {
        dest3[i] = source[3*i+2];
    }
}

/******************************************************************************/

// read and write two inputs at a time, cacheblocked
// helps on some cache architectures
template<typename T>
void deinterleave3to3_version4( const T *source, T *dest1, T *dest2,
                        T *dest3, int count ) {
    int i, j;
    const int blockSize = 4096/(3*sizeof(T));
    
    for ( i = 0; i < count; i += blockSize) {
        int block = blockSize;
        if ( (i+block) > count)
            block = count - i;
        
        for ( j = 0; j < block; ++j ) {
            dest1[j] = source[3*j+0];
            dest2[j] = source[3*j+1];
        }
        
        for ( j = 0; j < block; ++j ) {
            dest3[j] = source[3*j+2];
        }
        
        source += 3*block;
        dest1 += block;
        dest2 += block;
        dest3 += block;
    }
}

/******************************************************************************/

// read and write one input at a time, cacheblocked
// helps on some cache architectures
template<typename T>
void deinterleave3to3_version5( const T *source, T *dest1, T *dest2,
                        T *dest3, int count ) {
    int i, j;
    const int blockSize = 4096/(3*sizeof(T));
    
    for ( i = 0; i < count; i += blockSize) {
        int block = blockSize;
        if ( (i+block) > count)
            block = count - i;
        
        for ( j = 0; j < block; ++j ) {
            dest1[j] = source[3*j+0];
        }
        
        
        for ( j = 0; j < block; ++j ) {
            dest2[j] = source[3*j+1];
        }
        
        for ( j = 0; j < block; ++j ) {
            dest3[j] = source[3*j+2];
        }
        
        source += 3*block;
        dest1 += block;
        dest2 += block;
        dest3 += block;
    }
}

/******************************************************************************/
/******************************************************************************/

// straightforward implementation
template<typename T>
void deinterleave4to3_version1( const T *source, T *dest1, T *dest2,
                                T *dest3, int count ) {
    int i;
    for ( i = 0; i < count; ++i ) {
        // first value ignored
        dest1[i] = source[4*i+1];
        dest2[i] = source[4*i+2];
        dest3[i] = source[4*i+3];
    }
}

/******************************************************************************/

// read and write one input at a time
// helps on some cache architectures, hurts on others
template<typename T>
void deinterleave4to3_version2( const T *source, T *dest1, T *dest2,
                                T *dest3, int count ) {
    // first value ignored
    copy_with_stride( dest1, source+1, count, 1, 4 );
    copy_with_stride( dest2, source+2, count, 1, 4 );
    copy_with_stride( dest3, source+3, count, 1, 4 );
}

/******************************************************************************/

// read and write two inputs at a time
// helps on some cache architectures
template<typename T>
void deinterleave4to3_version3( const T *source, T *dest1, T *dest2,
                                T *dest3, int count ) {
    int i;
    for ( i = 0; i < count; ++i ) {
        dest1[i] = source[4*i+1];
        dest2[i] = source[4*i+2];
    }
    
    for ( i = 0; i < count; ++i ) {
        dest3[i] = source[4*i+3];
    }
}

/******************************************************************************/

// read and write one input at a time, cacheblocked
// helps on some cache architectures, hurts on others
template<typename T>
void deinterleave4to3_version4( const T *source, T *dest1, T *dest2,
                                T *dest3, int count ) {
    int i;
    const int blockSize = 4096/(4*sizeof(T));
    
    for ( i = 0; i < count; i += blockSize) {
        int block = blockSize;
        if ( (i+block) > count)
            block = count - i;
        
        // first value ignored
        copy_with_stride( dest1, source+1, block, 1, 4 );
        copy_with_stride( dest2, source+2, block, 1, 4 );
        copy_with_stride( dest3, source+3, block, 1, 4 );
        
        source += 4*block;
        dest1 += block;
        dest2 += block;
        dest3 += block;
    }
}

/******************************************************************************/

// read and write two inputs at a time, cacheblocked
// helps on some cache architectures
template<typename T>
void deinterleave4to3_version5( const T *source, T *dest1, T *dest2,
                                T *dest3, int count ) {
    int i, j;
    const int blockSize = 4096/(4*sizeof(T));
    
    for ( i = 0; i < count; i += blockSize) {
        int block = blockSize;
        if ( (i+block) > count)
            block = count - i;
        
        for ( j = 0; j < block; ++j ) {
            dest1[j] = source[4*j+1];
            dest2[j] = source[4*j+2];
        }
        
        for ( j = 0; j < block; ++j ) {
            dest3[j] = source[4*j+3];
        }
        
        source += 4*block;
        dest1 += block;
        dest2 += block;
        dest3 += block;
    }
}

/******************************************************************************/

// read and write one input at a time, cacheblocked
// helps on some cache architectures
template<typename T>
void deinterleave4to3_version6( const T *source, T *dest1, T *dest2,
                                T *dest3, int count ) {
    int i, j;
    const int blockSize = 4096/(4*sizeof(T));
    
    for ( i = 0; i < count; i += blockSize) {
        int block = blockSize;
        if ( (i+block) > count)
            block = count - i;
        
        for ( j = 0; j < block; ++j ) {
            dest1[j] = source[4*j+1];
        }
        
        for ( j = 0; j < block; ++j ) {
            dest2[j] = source[4*j+2];
        }
        
        for ( j = 0; j < block; ++j ) {
            dest3[j] = source[4*j+3];
        }
        
        source += 4*block;
        dest1 += block;
        dest2 += block;
        dest3 += block;
    }
}

/******************************************************************************/

// straightforward implementation
template<typename T>
void deinterleave4to4_version1( const T *source, T *dest1, T *dest2,
                                T *dest3, T *dest4, int count ) {
    int i;
    for ( i = 0; i < count; ++i ) {
        dest1[i] = source[4*i+0];
        dest2[i] = source[4*i+1];
        dest3[i] = source[4*i+2];
        dest4[i] = source[4*i+3];
    }
}

/******************************************************************************/

// read and write one input at a time
// helps on some cache architectures, hurts on others
template<typename T>
void deinterleave4to4_version2( const T *source, T *dest1, T *dest2,
                                T *dest3, T *dest4, int count ) {
    copy_with_stride( dest1, source+0, count, 1, 4 );
    copy_with_stride( dest2, source+1, count, 1, 4 );
    copy_with_stride( dest3, source+2, count, 1, 4 );
    copy_with_stride( dest4, source+3, count, 1, 4 );
}

/******************************************************************************/

// read and write two inputs at a time
// helps on some cache architectures
template<typename T>
void deinterleave4to4_version3( const T *source, T *dest1, T *dest2,
                                T *dest3, T *dest4, int count ) {
    int i;
    for ( i = 0; i < count; ++i ) {
        dest1[i] = source[4*i+0];
        dest2[i] = source[4*i+1];
    }
    
    for ( i = 0; i < count; ++i ) {
        dest3[i] = source[4*i+2];
        dest4[i] = source[4*i+3];
    }
}

/******************************************************************************/

// read and write one input at a time, cacheblocked
// helps on some cache architectures, hurts on others
template<typename T>
void deinterleave4to4_version4( const T *source, T *dest1, T *dest2,
                                T *dest3, T *dest4, int count ) {
    int i;
    const int blockSize = 4096/(4*sizeof(T));
    
    for ( i = 0; i < count; i += blockSize) {
        int block = blockSize;
        if ( (i+block) > count)
            block = count - i;
        
        copy_with_stride( dest1, source+0, block, 1, 4 );
        copy_with_stride( dest2, source+1, block, 1, 4 );
        copy_with_stride( dest3, source+2, block, 1, 4 );
        copy_with_stride( dest4, source+3, block, 1, 4 );
        
        source += 4*block;
        dest1 += block;
        dest2 += block;
        dest3 += block;
        dest4 += block;
    }
}

/******************************************************************************/

// read and write two inputs at a time, cacheblocked
// helps on some cache architectures
template<typename T>
void deinterleave4to4_version5( const T *source, T *dest1, T *dest2,
                                T *dest3, T *dest4, int count ) {
    int i, j;
    const int blockSize = 4096/(4*sizeof(T));
    
    for ( i = 0; i < count; i += blockSize) {
        int block = blockSize;
        if ( (i+block) > count)
            block = count - i;
        
        for ( j = 0; j < block; ++j ) {
            dest1[j] = source[4*j+0];
            dest2[j] = source[4*j+1];
        }
        
        for ( j = 0; j < block; ++j ) {
            dest3[j] = source[4*j+2];
            dest4[j] = source[4*j+3];
        }
        
        source += 4*block;
        dest1 += block;
        dest2 += block;
        dest3 += block;
        dest4 += block;
    }
}

/******************************************************************************/

// read and write one inputs at a time, cacheblocked
// helps on some cache architectures
template<typename T>
void deinterleave4to4_version6( const T *source, T *dest1, T *dest2,
                                T *dest3, T *dest4, int count ) {
    int i, j;
    const int blockSize = 4096/(4*sizeof(T));
    
    for ( i = 0; i < count; i += blockSize) {
        int block = blockSize;
        if ( (i+block) > count)
            block = count - i;
        
        for ( j = 0; j < block; ++j ) {
            dest1[j] = source[4*j+0];
        }
        
        for ( j = 0; j < block; ++j ) {
            dest2[j] = source[4*j+1];
        }
        
        for ( j = 0; j < block; ++j ) {
            dest3[j] = source[4*j+2];
        }
        
        for ( j = 0; j < block; ++j ) {
            dest4[j] = source[4*j+3];
        }
        
        source += 4*block;
        dest1 += block;
        dest2 += block;
        dest3 += block;
        dest4 += block;
    }
}

/******************************************************************************/
/******************************************************************************/

static std::deque<std::string> gLabels;

template <typename T, typename Move >
void test_deinterleave2to2(const T *source, T *dest1, T *dest2,
                    int count, Move deinterleave2to2, const std::string label) {
    int i;
    
    // make sure we are verifying this run correctly
    fill( dest1, dest1+count, T(0) );
    fill( dest2, dest2+count, T(0) );
    
    start_timer();

    for(i = 0; i < iterations; ++i) {
        deinterleave2to2( source, dest1, dest2, count );
    }
    
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
    
    T *destList[2];
    destList[0] = dest1;
    destList[1] = dest2;
    verify_deinterleave_list( source, destList, count, 2, label );
}

/******************************************************************************/

template <typename T, typename Move >
void test_deinterleave3to3(const T *source, T *dest1, T *dest2,
                    T *dest3, int count,
                    Move deinterleave3to3, const std::string label) {
    int i;
    
    // make sure we are verifying this run correctly
    fill( dest1, dest1+count, T(0) );
    fill( dest2, dest2+count, T(0) );
    fill( dest3, dest3+count, T(0) );
    
    start_timer();

    for(i = 0; i < iterations; ++i) {
        deinterleave3to3( source, dest1, dest2, dest3, count );
    }
    
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
    
    T *destList[3];
    destList[0] = dest1;
    destList[1] = dest2;
    destList[2] = dest3;
    verify_deinterleave_list( source, destList, count, 3, label );
}

/******************************************************************************/

template <typename T, typename Move >
void test_deinterleave4to3(const T *source, T *dest1,
                    T *dest2, T *dest3, T *dest4, int count,
                    Move deinterleave4to3, const std::string label) {
    int i;
    
    // make sure we are verifying this run correctly
    fill( dest1, dest1+count, T(0) );
    fill( dest2, dest2+count, T(0) );
    fill( dest3, dest3+count, T(0) );
    
    start_timer();

    for(i = 0; i < iterations; ++i) {
        deinterleave4to3( source, dest1, dest2, dest3, count );
    }
    
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );

    T *destList[4];
    destList[0] = NULL;
    destList[1] = dest1;
    destList[2] = dest2;
    destList[3] = dest3;
    verify_deinterleave_list( source, destList, count, 4, label );
}

/******************************************************************************/

template <typename T, typename Move >
void test_deinterleave4to4(const T *source, T *dest1, T *dest2,
                    T *dest3, T *dest4, int count,
                    Move deinterleave4to4, const std::string label) {
    int i;
    
    // make sure we are verifying this run correctly
    fill( dest1, dest1+count, T(0) );
    fill( dest2, dest2+count, T(0) );
    fill( dest3, dest3+count, T(0) );
    fill( dest4, dest4+count, T(0) );
    
    start_timer();

    for(i = 0; i < iterations; ++i) {
        deinterleave4to4( source, dest1, dest2, dest3, dest4, count );
    }
    
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
    
    T *destList[4];
    destList[0] = dest1;
    destList[1] = dest2;
    destList[2] = dest3;
    destList[3] = dest4;
    verify_deinterleave_list( source, destList, count, 4, label );
}

/******************************************************************************/
/******************************************************************************/

template< typename T >
void TestOneType()
{
    std::string myTypeName( getTypeName<T>() );
    
    gLabels.clear();
    
    scrand( init_value+420 );
    
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


    fill_random( dataE, dataE+4*SIZE );
    
    fill( dataA, dataA+SIZE, T(0) );
    fill( dataB, dataB+SIZE, T(0) );
    fill( dataC, dataC+SIZE, T(0) );
    fill( dataD, dataD+SIZE, T(0) );


    iterations = base_iterations;
    test_deinterleave2to2( dataE, dataA, dataB, SIZE, deinterleave2to2_version1<T>, myTypeName + " deinterleave2to2_1");
    test_deinterleave2to2( dataE, dataA, dataB, SIZE, deinterleave2to2_version2<T>, myTypeName + " deinterleave2to2_2");
    test_deinterleave2to2( dataE, dataA, dataB, SIZE, deinterleave2to2_version3<T>, myTypeName + " deinterleave2to2_3");
    test_deinterleave2to2( dataE, dataA, dataB, SIZE, deinterleave2to2_version4<T>, myTypeName + " deinterleave2to2_4");
    test_deinterleave2to2( dataE, dataA, dataB, SIZE, deinterleave2to2_version5<T>, myTypeName + " deinterleave2to2_5");
    test_deinterleave2to2( dataE, dataA, dataB, SIZE, deinterleave2to2_version6<T>, myTypeName + " deinterleave2to2_6");
    test_deinterleave2to2( dataE, dataA, dataB, SIZE, deinterleave2to2_version7<T>, myTypeName + " deinterleave2to2_7");
    test_deinterleave2to2( dataE, dataA, dataB, SIZE, deinterleave2to2_version8<T>, myTypeName + " deinterleave2to2_8");
    
    std::string temp1( myTypeName + " deinterleave2to2" );
    summarize(temp1.c_str(), SIZE, iterations, kDontShowGMeans, kDontShowPenalty );


    iterations = base_iterations / 2;
    test_deinterleave3to3( dataE, dataA, dataB, dataC, SIZE, deinterleave3to3_version1<T>, myTypeName + " deinterleave3to3_1");
    test_deinterleave3to3( dataE, dataA, dataB, dataC, SIZE, deinterleave3to3_version2<T>, myTypeName + " deinterleave3to3_2");
    test_deinterleave3to3( dataE, dataA, dataB, dataC, SIZE, deinterleave3to3_version3<T>, myTypeName + " deinterleave3to3_3");
    test_deinterleave3to3( dataE, dataA, dataB, dataC, SIZE, deinterleave3to3_version4<T>, myTypeName + " deinterleave3to3_4");
    test_deinterleave3to3( dataE, dataA, dataB, dataC, SIZE, deinterleave3to3_version5<T>, myTypeName + " deinterleave3to3_5");
    
    std::string temp2( myTypeName + " deinterleave3to3" );
    summarize(temp2.c_str(), SIZE, iterations, kDontShowGMeans, kDontShowPenalty );


    iterations = base_iterations / 2;
    test_deinterleave4to3( dataE, dataA, dataB, dataC, dataD, SIZE, deinterleave4to3_version1<T>, myTypeName + " deinterleave4to3_1");
    test_deinterleave4to3( dataE, dataA, dataB, dataC, dataD, SIZE, deinterleave4to3_version2<T>, myTypeName + " deinterleave4to3_2");
    test_deinterleave4to3( dataE, dataA, dataB, dataC, dataD, SIZE, deinterleave4to3_version3<T>, myTypeName + " deinterleave4to3_3");
    test_deinterleave4to3( dataE, dataA, dataB, dataC, dataD, SIZE, deinterleave4to3_version4<T>, myTypeName + " deinterleave4to3_4");
    test_deinterleave4to3( dataE, dataA, dataB, dataC, dataD, SIZE, deinterleave4to3_version5<T>, myTypeName + " deinterleave4to3_5");
    test_deinterleave4to3( dataE, dataA, dataB, dataC, dataD, SIZE, deinterleave4to3_version6<T>, myTypeName + " deinterleave4to3_6");
    
    std::string temp3( myTypeName + " deinterleave4to3" );
    summarize(temp3.c_str(), SIZE, iterations, kDontShowGMeans, kDontShowPenalty );


    iterations = base_iterations / 2;
    test_deinterleave4to4( dataE, dataA, dataB, dataC, dataD, SIZE, deinterleave4to4_version1<T>, myTypeName + " deinterleave4to4_1");
    test_deinterleave4to4( dataE, dataA, dataB, dataC, dataD, SIZE, deinterleave4to4_version2<T>, myTypeName + " deinterleave4to4_2");
    test_deinterleave4to4( dataE, dataA, dataB, dataC, dataD, SIZE, deinterleave4to4_version3<T>, myTypeName + " deinterleave4to4_3");
    test_deinterleave4to4( dataE, dataA, dataB, dataC, dataD, SIZE, deinterleave4to4_version4<T>, myTypeName + " deinterleave4to4_4");
    test_deinterleave4to4( dataE, dataA, dataB, dataC, dataD, SIZE, deinterleave4to4_version5<T>, myTypeName + " deinterleave4to4_5");
    test_deinterleave4to4( dataE, dataA, dataB, dataC, dataD, SIZE, deinterleave4to4_version6<T>, myTypeName + " deinterleave4to4_6");
    
    std::string temp4( myTypeName + " deinterleave4to4" );
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

// signed versus unsigned doesn't matter for this

    TestOneType<uint8_t>();
    iterations /= 2;
    TestOneType<uint16_t>();
    iterations /= 2;
    TestOneType<uint32_t>();
    iterations /= 2;
    TestOneType<uint64_t>();
    TestOneType<float>();
    TestOneType<double>();
    iterations /= 2;
    TestOneType<long double>();


    return 0;
}

// the end
/******************************************************************************/
/******************************************************************************/
