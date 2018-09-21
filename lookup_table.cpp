/*
    Copyright 2008-2009 Adobe Systems Incorporated
    Copyright 2018 Chris Cox
    Distributed under the MIT License (see accompanying file LICENSE_1_0_0.txt
    or a copy at http://stlab.adobe.com/licenses.html )


Goal: Test performance of various idioms and optimizations for lookup tables.


Assumptions:
    1) The compiler will optimize lookup table operations.
        Unrolling will usually be needed to hide read latencies.

    2) The compiler should recognize ineffecient lookup table idioms and substitute efficient methods.
        Many different CPU architecture issues will require reading and writing words for best performance.
            CPUs with...
                    cache write-back/write-combine delays.
                    store forwarding delays.
                    slow cache access relative to shifts/masks.
                    slow partial word (byte) access.
                    fast shift/mask operations.
        On some CPUs, a lookup can be handled with vector instructions.
        On some CPUs, special cache handling is needed (especially 2way caches).




TODO - lookup and interpolate (int16_t, int32_t, int64_t, float, double)
TODO - 2D and 3D LUTs, simple and interpolated

*/

/******************************************************************************/

#include "benchmark_stdint.hpp"
#include <cstddef>
#include <cstdio>
#include <ctime>
#include <cmath>
#include <cstdlib>
#include <algorithm>
#include "benchmark_results.h"
#include "benchmark_timer.h"
#include "benchmark_algorithms.h"

/******************************************************************************/
/******************************************************************************/

// this constant may need to be adjusted to give reasonable minimum times
// For best results, times should be about 1.0 seconds for the minimum test run
int base_iterations = 800000;
int iterations = base_iterations;

// 4000 items, or about 4..8k of data
// this is intended to remain within the L1 cache of most common CPUs
const int SIZE_SMALL = 4000;

// about 8..16M of data
// this is intended to be outside the L2 cache of most common CPUs
const int SIZE = 8000000;

// initial value for filling our arrays, may be changed from the command line
int32_t init_value = 3;

/******************************************************************************/

// our global arrays of numbers

uint8_t inputData8[SIZE];
uint8_t resultData8[SIZE];

uint16_t inputData16[SIZE];
uint16_t resultData16[SIZE];

/******************************************************************************/
/******************************************************************************/

template<typename T>
inline void verify_LUT(T* result, int count, const char *label) {
    int j;

    for (j = 0; j < count; ++j) {
        if (result[j] != T(init_value)) {
            printf("test %s failed (got %u, expected %u)\n", label, unsigned(result[j]), unsigned(init_value));
            break;
        }
    }
}

/******************************************************************************/
/******************************************************************************/

// baseline - a trivial loop
template <typename T>
void apply_lut1(const T* input, T *result, const int count, const T* LUT) {
    for (int j = 0; j < count; ++j) {
        result[j] = LUT[ input[j] ];
    }
}

/******************************************************************************/

// baseline - a trivial loop
template <typename T>
void test_lut1(const T* input, T *result, const int count, const T* LUT, const char *label) {

    start_timer();

    for(int i = 0; i < iterations; ++i) {
        apply_lut1(input,result,count,LUT);
    }
    
    record_result( timer(), label );
    verify_LUT(result,count, label);
}

/******************************************************************************/

// trivial loop, expanded operations
// NOTE - this should generate the same code as the previous version
template <typename T>
void test_lut2(const T* input, T *result, const int count, const T* LUT, const char *label) {
    int i, j;

    start_timer();

    for(i = 0; i < iterations; ++i) {
        for (j = 0; j < count; ++j) {
            T old_value = input[j];
            T new_value = LUT[ old_value ];
            result[j] = new_value;
        }
    }

    record_result( timer(), label );
    verify_LUT(result,count, label);
}

/******************************************************************************/

// unroll 4X
template <typename T>
void test_lut3(const T* input, T *result, const int count, const T* LUT, const char *label) {
    int i, j;

    start_timer();

    for(i = 0; i < iterations; ++i) {
        for (j = 0; j < (count - 3); j += 4) {
            T value0 = input[j+0];
            T value1 = input[j+1];
            T value2 = input[j+2];
            T value3 = input[j+3];
            
            value0 = LUT[ value0 ];
            value1 = LUT[ value1 ];
            value2 = LUT[ value2 ];
            value3 = LUT[ value3 ];
            
            result[j+0] = value0;
            result[j+1] = value1;
            result[j+2] = value2;
            result[j+3] = value3;
        }
        for (; j < count; ++j) {
            result[j] = LUT[ input[j] ];
        }
    }

    record_result( timer(), label );
    verify_LUT(result,count, label);
}

/******************************************************************************/

// read and write 32 bit values
// must be special cased for each data size
// NOTE - this may be slower without correct loop unrolling
void test_lut4(const uint8_t* input, uint8_t *result, const int count, const uint8_t* LUT, const char *label) {
    int i, j;

    start_timer();

    for(i = 0; i < iterations; ++i) {
        j = 0;
        
        // align to 32 bit boundary on destination
        for (; j < count && ((intptr_t(result) & 3) != 0); ++j) {
            result[j] = LUT[ input[j] ];
        }
        
        // read and write 32 bits at a time
        for (; j < (count - 3); j += 4) {
            uint32_t longValue = *( (uint32_t *)(input + j) );
            
            uint8_t value0 = (longValue >> 24) & 0xFF;
            uint8_t value1 = (longValue >> 16) & 0xFF;
            uint8_t value2 = (longValue >> 8) & 0xFF;
            uint8_t value3 = longValue & 0xFF;
            
            value0 = LUT[ value0 ];
            value1 = LUT[ value1 ];
            value2 = LUT[ value2 ];
            value3 = LUT[ value3 ];
            
            uint32_t longResult = (value0 << 24) | (value1 << 16) | (value2 << 8) | value3;
            
            *( (uint32_t *)(result + j) ) = longResult;
        }
        
        // handle remaining bytes
        for (; j < count; ++j) {
            result[j] = LUT[ input[j] ];
        }
    }

    record_result( timer(), label );
    verify_LUT(result,count, label);
}

/******************************************************************************/

// read and write 32 bit values
// must be special cased for each data size
// NOTE - this may be slower without correct loop unrolling
void test_lut4(const int8_t* input, int8_t *result, const int count, const int8_t* LUT, const char *label) {
    int i, j;

    start_timer();

    for(i = 0; i < iterations; ++i) {
        j = 0;
        
        // align to 32 bit boundary on destination
        for (; j < count && ((intptr_t(result) & 3) != 0); ++j) {
            result[j] = LUT[ input[j] ];
        }
        
        // read and write 32 bits at a time
        for (; j < (count - 3); j += 4) {
            uint32_t longValue = *( (uint32_t *)(input + j) );
            
            int8_t value0 = (longValue >> 24) & 0xFF;
            int8_t value1 = (longValue >> 16) & 0xFF;
            int8_t value2 = (longValue >> 8) & 0xFF;
            int8_t value3 = longValue & 0xFF;
            
            value0 = LUT[ value0 ];
            value1 = LUT[ value1 ];
            value2 = LUT[ value2 ];
            value3 = LUT[ value3 ];
            
            uint32_t longResult = ((int32_t)value0 << 24) | ((int32_t)value1 << 16) | ((int32_t)value2 << 8) | value3;
            
            *( (uint32_t *)(result + j) ) = longResult;
        }
        
        // handle remaining bytes
        for (; j < count; ++j) {
            result[j] = LUT[ input[j] ];
        }
    }

    record_result( timer(), label );
    verify_LUT(result,count, label);
}


/******************************************************************************/

// read and write 32 bit values
// must be special cased for each data size
// NOTE - this may be slower without correct loop unrolling
void test_lut4(const uint16_t* input, uint16_t *result, const int count, const uint16_t* LUT, const char *label) {
    int i, j;

    start_timer();

    for(i = 0; i < iterations; ++i) {
        j = 0;
        
        // align to 32 bit boundary on destination
        for (; j < count && ((intptr_t(result) & 3) != 0); ++j) {
            result[j] = LUT[ input[j] ];
        }
        
        // read and write 32 bits at a time
        for (; j < (count - 3); j += 4) {
            uint32_t longValue = *( (uint32_t *)(input + j) );
            
            uint32_t value0 = (longValue >> 16) & 0xFFFF;
            uint32_t value1 = longValue & 0xFFFF;
            
            value0 = LUT[ value0 ];
            value1 = LUT[ value1 ];
            
            uint32_t longResult = (value0 << 16) | value1;
            
            *( (uint32_t *)(result + j) ) = longResult;
        }
        
        // handle remaining bytes
        for (; j < count; ++j) {
            result[j] = LUT[ input[j] ];
        }
    }

    record_result( timer(), label );
    verify_LUT(result,count, label);
}


/******************************************************************************/

// read and write 32 bit values
// must be special cased for each data size
// NOTE - this may be slower without correct loop unrolling
void test_lut4(const int16_t* input, int16_t *result, const int count, const int16_t* LUT, const char *label) {
    int i, j;

    start_timer();

    for(i = 0; i < iterations; ++i) {
        j = 0;
        
        // align to 32 bit boundary on destination
        for (; j < count && ((intptr_t(result) & 3) != 0); ++j) {
            result[j] = LUT[ input[j] ];
        }
        
        // read and write 32 bits at a time
        for (; j < (count - 3); j += 4) {
            uint32_t longValue = *( (uint32_t *)(input + j) );
            
            int32_t value0 = (longValue >> 16) & 0xFFFF;
            int32_t value1 = longValue & 0xFFFF;
            
            value0 = LUT[ value0 ];
            value1 = LUT[ value1 ];
            
            uint32_t longResult = (value0 << 16) | value1;
            
            *( (uint32_t *)(result + j) ) = longResult;
        }
        
        // handle remaining bytes
        for (; j < count; ++j) {
            result[j] = LUT[ input[j] ];
        }
    }

    record_result( timer(), label );
    verify_LUT(result,count, label);
}

/******************************************************************************/

// read and write 32 bit values, and unroll 4X
// must be special cased for each data size
void test_lut5(const uint8_t* input, uint8_t *result, const int count, const uint8_t* LUT, const char *label) {
    int i, j;

    start_timer();

    for(i = 0; i < iterations; ++i) {
        j = 0;
        
        // align to 32 bit boundary on destination
        for ( ; j < count && ((intptr_t(result) & 3) != 0); ++j) {
            result[j] = LUT[ input[j] ];
        }
        
        // read and write 32 bits at a time, unrolled 4X
        for ( ; j < (count - 15); j += 16) {
            uint32_t longValue0 = *( (uint32_t *)(input + j + 0) );
            uint32_t longValue1 = *( (uint32_t *)(input + j + 4) );
            uint32_t longValue2 = *( (uint32_t *)(input + j + 8) );
            uint32_t longValue3 = *( (uint32_t *)(input + j + 12) );
            
            uint32_t value0 = (longValue0 >> 24) & 0xFF;
            uint32_t value1 = (longValue0 >> 16) & 0xFF;
            uint32_t value2 = (longValue0 >> 8) & 0xFF;
            uint32_t value3 = longValue0 & 0xFF;
            
            value0 = LUT[ value0 ];
            value1 = LUT[ value1 ];
            value2 = LUT[ value2 ];
            value3 = LUT[ value3 ];
            
            uint32_t value4 = (longValue1 >> 24) & 0xFF;
            uint32_t value5 = (longValue1 >> 16) & 0xFF;
            uint32_t value6 = (longValue1 >> 8) & 0xFF;
            uint32_t value7 = longValue1 & 0xFF;
            
            value4 = LUT[ value4 ];
            value5 = LUT[ value5 ];
            value6 = LUT[ value6 ];
            value7 = LUT[ value7 ];
            
            uint32_t value8 = (longValue2 >> 24) & 0xFF;
            uint32_t value9 = (longValue2 >> 16) & 0xFF;
            uint32_t valueA = (longValue2 >> 8) & 0xFF;
            uint32_t valueB = longValue2 & 0xFF;
            
            value8 = LUT[ value8 ];
            value9 = LUT[ value9 ];
            valueA = LUT[ valueA ];
            valueB = LUT[ valueB ];
            
            uint32_t valueC = (longValue3 >> 24) & 0xFF;
            uint32_t valueD = (longValue3 >> 16) & 0xFF;
            uint32_t valueE = (longValue3 >> 8) & 0xFF;
            uint32_t valueF = longValue3 & 0xFF;
            
            valueC = LUT[ valueC ];
            valueD = LUT[ valueD ];
            valueE = LUT[ valueE ];
            valueF = LUT[ valueF ];
            
            uint32_t longResult0 = (value0 << 24) | (value1 << 16) | (value2 << 8) | value3;
            uint32_t longResult1 = (value4 << 24) | (value5 << 16) | (value6 << 8) | value7;
            uint32_t longResult2 = (value8 << 24) | (value9 << 16) | (valueA << 8) | valueB;
            uint32_t longResult3 = (valueC << 24) | (valueD << 16) | (valueE << 8) | valueF;
            
            *( (uint32_t *)(result + j + 0) ) = longResult0;
            *( (uint32_t *)(result + j + 4) ) = longResult1;
            *( (uint32_t *)(result + j + 8) ) = longResult2;
            *( (uint32_t *)(result + j + 12) ) = longResult3;
        }
        
        // handle remaining bytes
        for ( ; j < count; ++j) {
            result[j] = LUT[ input[j] ];
        }
    }

    record_result( timer(), label );
    verify_LUT(result,count, label);
}

/******************************************************************************/

// read and write 32 bit values, and unroll 4X
// must be special cased for each data size
void test_lut5(const int8_t* input, int8_t *result, const int count, const int8_t* LUT, const char *label) {
    int i, j;

    start_timer();

    for(i = 0; i < iterations; ++i) {
        j = 0;
        
        // align to 32 bit boundary on destination
        for ( ; j < count && ((intptr_t(result) & 3) != 0); ++j) {
            result[j] = LUT[ input[j] ];
        }
        
        // read and write 32 bits at a time, unrolled 4X
        for ( ; j < (count - 15); j += 16) {
            uint32_t longValue0 = *( (uint32_t *)(input + j + 0) );
            uint32_t longValue1 = *( (uint32_t *)(input + j + 4) );
            uint32_t longValue2 = *( (uint32_t *)(input + j + 8) );
            uint32_t longValue3 = *( (uint32_t *)(input + j + 12) );
            
            int8_t value0 = (longValue0 >> 24) & 0xFF;
            int8_t value1 = (longValue0 >> 16) & 0xFF;
            int8_t value2 = (longValue0 >> 8) & 0xFF;
            int8_t value3 = longValue0 & 0xFF;
            
            value0 = LUT[ value0 ];
            value1 = LUT[ value1 ];
            value2 = LUT[ value2 ];
            value3 = LUT[ value3 ];
            
            int8_t value4 = (longValue1 >> 24) & 0xFF;
            int8_t value5 = (longValue1 >> 16) & 0xFF;
            int8_t value6 = (longValue1 >> 8) & 0xFF;
            int8_t value7 = longValue1 & 0xFF;
            
            value4 = LUT[ value4 ];
            value5 = LUT[ value5 ];
            value6 = LUT[ value6 ];
            value7 = LUT[ value7 ];
            
            int8_t value8 = (longValue2 >> 24) & 0xFF;
            int8_t value9 = (longValue2 >> 16) & 0xFF;
            int8_t valueA = (longValue2 >> 8) & 0xFF;
            int8_t valueB = longValue2 & 0xFF;
            
            value8 = LUT[ value8 ];
            value9 = LUT[ value9 ];
            valueA = LUT[ valueA ];
            valueB = LUT[ valueB ];
            
            int8_t valueC = (longValue3 >> 24) & 0xFF;
            int8_t valueD = (longValue3 >> 16) & 0xFF;
            int8_t valueE = (longValue3 >> 8) & 0xFF;
            int8_t valueF = longValue3 & 0xFF;
            
            valueC = LUT[ valueC ];
            valueD = LUT[ valueD ];
            valueE = LUT[ valueE ];
            valueF = LUT[ valueF ];
            
            uint32_t longResult0 = ((int32_t)value0 << 24) | ((int32_t)value1 << 16) | ((int32_t)value2 << 8) | value3;
            uint32_t longResult1 = ((int32_t)value4 << 24) | ((int32_t)value5 << 16) | ((int32_t)value6 << 8) | value7;
            uint32_t longResult2 = ((int32_t)value8 << 24) | ((int32_t)value9 << 16) | ((int32_t)valueA << 8) | valueB;
            uint32_t longResult3 = ((int32_t)valueC << 24) | ((int32_t)valueD << 16) | ((int32_t)valueE << 8) | valueF;
            
            *( (uint32_t *)(result + j + 0) ) = longResult0;
            *( (uint32_t *)(result + j + 4) ) = longResult1;
            *( (uint32_t *)(result + j + 8) ) = longResult2;
            *( (uint32_t *)(result + j + 12) ) = longResult3;
        }
        
        // handle remaining bytes
        for ( ; j < count; ++j) {
            result[j] = LUT[ input[j] ];
        }
    }

    record_result( timer(), label );
    verify_LUT(result,count, label);
}


/******************************************************************************/

// read and write 32 bit values, and unroll 4X
// must be special cased for each data size
void test_lut5(const uint16_t* input, uint16_t *result, const int count, const uint16_t* LUT, const char *label) {
    int i, j;

    start_timer();

    for(i = 0; i < iterations; ++i) {
        j = 0;
        
        // align to 32 bit boundary on destination
        for ( ; j < count && ((intptr_t(result) & 3) != 0); ++j) {
            result[j] = LUT[ input[j] ];
        }
        
        // read and write 32 bits at a time, unrolled 4X
        for ( ; j < (count - 15); j += 16) {
            uint32_t longValue0 = *( (uint32_t *)(input + j + 0) );
            uint32_t longValue1 = *( (uint32_t *)(input + j + 4) );
            uint32_t longValue2 = *( (uint32_t *)(input + j + 8) );
            uint32_t longValue3 = *( (uint32_t *)(input + j + 12) );
            
            uint32_t value0 = (longValue0 >> 16) & 0xFFFF;
            uint32_t value1 = longValue0 & 0xFFFF;
            uint32_t value2 = (longValue1 >> 16) & 0xFFFF;
            uint32_t value3 = longValue1 & 0xFFFF;
            uint32_t value4 = (longValue2 >> 16) & 0xFFFF;
            uint32_t value5 = longValue2 & 0xFFFF;
            uint32_t value6 = (longValue3 >> 16) & 0xFFFF;
            uint32_t value7 = longValue3 & 0xFFFF;
            
            value0 = LUT[ value0 ];
            value1 = LUT[ value1 ];
            value2 = LUT[ value2 ];
            value3 = LUT[ value3 ];
            
            value4 = LUT[ value4 ];
            value5 = LUT[ value5 ];
            value6 = LUT[ value6 ];
            value7 = LUT[ value7 ];
            
            uint32_t longResult0 = (value0 << 16) | value1;
            uint32_t longResult1 = (value2 << 16) | value3;
            uint32_t longResult2 = (value4 << 16) | value5;
            uint32_t longResult3 = (value6 << 16) | value7;
            
            *( (uint32_t *)(result + j + 0) ) = longResult0;
            *( (uint32_t *)(result + j + 4) ) = longResult1;
            *( (uint32_t *)(result + j + 8) ) = longResult2;
            *( (uint32_t *)(result + j + 12) ) = longResult3;
        }
        
        // handle remaining bytes
        for ( ; j < count; ++j) {
            result[j] = LUT[ input[j] ];
        }
    }

    record_result( timer(), label );
    verify_LUT(result,count, label);
}


/******************************************************************************/

// read and write 32 bit values, and unroll 4X
// must be special cased for each data size
void test_lut5(const int16_t* input, int16_t *result, const int count, const int16_t* LUT, const char *label) {
    int i, j;

    start_timer();

    for(i = 0; i < iterations; ++i) {
        j = 0;
        
        // align to 32 bit boundary on destination
        for ( ; j < count && ((intptr_t(result) & 3) != 0); ++j) {
            result[j] = LUT[ input[j] ];
        }
        
        // read and write 32 bits at a time, unrolled 4X
        for ( ; j < (count - 15); j += 16) {
            uint32_t longValue0 = *( (uint32_t *)(input + j + 0) );
            uint32_t longValue1 = *( (uint32_t *)(input + j + 4) );
            uint32_t longValue2 = *( (uint32_t *)(input + j + 8) );
            uint32_t longValue3 = *( (uint32_t *)(input + j + 12) );
            
            int16_t value0 = (longValue0 >> 16) & 0xFFFF;
            int16_t value1 = longValue0 & 0xFFFF;
            int16_t value2 = (longValue1 >> 16) & 0xFFFF;
            int16_t value3 = longValue1 & 0xFFFF;
            int16_t value4 = (longValue2 >> 16) & 0xFFFF;
            int16_t value5 = longValue2 & 0xFFFF;
            int16_t value6 = (longValue3 >> 16) & 0xFFFF;
            int16_t value7 = longValue3 & 0xFFFF;
            
            value0 = LUT[ value0 ];
            value1 = LUT[ value1 ];
            value2 = LUT[ value2 ];
            value3 = LUT[ value3 ];
            
            value4 = LUT[ value4 ];
            value5 = LUT[ value5 ];
            value6 = LUT[ value6 ];
            value7 = LUT[ value7 ];
            
            uint32_t longResult0 = ((int32_t)value0 << 16) | value1;
            uint32_t longResult1 = ((int32_t)value2 << 16) | value3;
            uint32_t longResult2 = ((int32_t)value4 << 16) | value5;
            uint32_t longResult3 = ((int32_t)value6 << 16) | value7;
            
            *( (uint32_t *)(result + j + 0) ) = longResult0;
            *( (uint32_t *)(result + j + 4) ) = longResult1;
            *( (uint32_t *)(result + j + 8) ) = longResult2;
            *( (uint32_t *)(result + j + 12) ) = longResult3;
        }
        
        // handle remaining bytes
        for ( ; j < count; ++j) {
            result[j] = LUT[ input[j] ];
        }
    }

    record_result( timer(), label );
    verify_LUT(result,count, label);
}

/******************************************************************************/

// read and write 64 bit values, and unroll 2X
// must be special cased for each data size
// NOTE - on a 32 bit CPU this should generate almost the same code as the previous version (assuming correct optimization)
void test_lut6(const uint8_t* input, uint8_t *result, const int count, const uint8_t* LUT, const char *label) {
    int i, j;

    start_timer();

    for(i = 0; i < iterations; ++i) {
        j = 0;
        
        // align to 64 bit boundary on destination
        for ( ; j < count && ((intptr_t(result) & 7) != 0); ++j) {
            result[j] = LUT[ input[j] ];
        }
        
        // read and write 64 bits at a time, unrolled 2X
        for ( ; j < (count - 15); j += 16) {
            uint64_t longValue0 = *( (uint64_t *)(input + j + 0) );
            uint64_t longValue1 = *( (uint64_t *)(input + j + 8) );
            
            uint64_t value0 = (longValue0 >> 56) & 0xFF;
            uint64_t value1 = (longValue0 >> 48) & 0xFF;
            uint64_t value2 = (longValue0 >> 40) & 0xFF;
            uint64_t value3 = (longValue0 >> 32) & 0xFF;
            uint64_t value4 = (longValue0 >> 24) & 0xFF;
            uint64_t value5 = (longValue0 >> 16) & 0xFF;
            uint64_t value6 = (longValue0 >> 8) & 0xFF;
            uint64_t value7 = longValue0 & 0xFF;
            
            value0 = LUT[ value0 ];
            value1 = LUT[ value1 ];
            value2 = LUT[ value2 ];
            value3 = LUT[ value3 ];
            value4 = LUT[ value4 ];
            value5 = LUT[ value5 ];
            value6 = LUT[ value6 ];
            value7 = LUT[ value7 ];
            
            uint64_t value8 = (longValue1 >> 56) & 0xFF;
            uint64_t value9 = (longValue1 >> 48) & 0xFF;
            uint64_t valueA = (longValue1 >> 40) & 0xFF;
            uint64_t valueB = (longValue1 >> 32) & 0xFF;
            uint64_t valueC = (longValue1 >> 24) & 0xFF;
            uint64_t valueD = (longValue1 >> 16) & 0xFF;
            uint64_t valueE = (longValue1 >> 8) & 0xFF;
            uint64_t valueF = longValue1 & 0xFF;
            
            value8 = LUT[ value8 ];
            value9 = LUT[ value9 ];
            valueA = LUT[ valueA ];
            valueB = LUT[ valueB ];
            valueC = LUT[ valueC ];
            valueD = LUT[ valueD ];
            valueE = LUT[ valueE ];
            valueF = LUT[ valueF ];
            
            uint64_t longResult0 = (value0 << 56) | (value1 << 48) | (value2 << 40) | (value3 << 32)
                                | (value4 << 24) | (value5 << 16) | (value6 << 8) | value7;
            uint64_t longResult1 = (value8 << 56) | (value9 << 48) | (valueA << 40) | (valueB << 32)
                                | (valueC << 24) | (valueD << 16) | (valueE << 8) | valueF;
            
            *( (uint64_t *)(result + j + 0) ) = longResult0;
            *( (uint64_t *)(result + j + 8) ) = longResult1;
        }
        
        // handle remaining bytes
        for ( ; j < count; ++j) {
            result[j] = LUT[ input[j] ];
        }
    }

    record_result( timer(), label );
    verify_LUT(result,count, label);
}


/******************************************************************************/

// read and write 64 bit values, and unroll 2X
// must be special cased for each data size
// NOTE - on a 32 bit CPU this should generate almost the same code as the previous version (assuming correct optimization)
void test_lut6(const int8_t* input, int8_t *result, const int count, const int8_t* LUT, const char *label) {
    int i, j;

    start_timer();

    for(i = 0; i < iterations; ++i) {
        j = 0;
        
        // align to 64 bit boundary on destination
        for ( ; j < count && ((intptr_t(result) & 7) != 0); ++j) {
            result[j] = LUT[ input[j] ];
        }
        
        // read and write 64 bits at a time, unrolled 2X
        for ( ; j < (count - 15); j += 16) {
            uint64_t longValue0 = *( (uint64_t *)(input + j + 0) );
            uint64_t longValue1 = *( (uint64_t *)(input + j + 8) );
            
            int8_t value0 = (longValue0 >> 56) & 0xFF;
            int8_t value1 = (longValue0 >> 48) & 0xFF;
            int8_t value2 = (longValue0 >> 40) & 0xFF;
            int8_t value3 = (longValue0 >> 32) & 0xFF;
            int8_t value4 = (longValue0 >> 24) & 0xFF;
            int8_t value5 = (longValue0 >> 16) & 0xFF;
            int8_t value6 = (longValue0 >> 8) & 0xFF;
            int8_t value7 = longValue0 & 0xFF;
            
            value0 = LUT[ value0 ];
            value1 = LUT[ value1 ];
            value2 = LUT[ value2 ];
            value3 = LUT[ value3 ];
            value4 = LUT[ value4 ];
            value5 = LUT[ value5 ];
            value6 = LUT[ value6 ];
            value7 = LUT[ value7 ];
            
            int8_t value8 = (longValue1 >> 56) & 0xFF;
            int8_t value9 = (longValue1 >> 48) & 0xFF;
            int8_t valueA = (longValue1 >> 40) & 0xFF;
            int8_t valueB = (longValue1 >> 32) & 0xFF;
            int8_t valueC = (longValue1 >> 24) & 0xFF;
            int8_t valueD = (longValue1 >> 16) & 0xFF;
            int8_t valueE = (longValue1 >> 8) & 0xFF;
            int8_t valueF = longValue1 & 0xFF;
            
            value8 = LUT[ value8 ];
            value9 = LUT[ value9 ];
            valueA = LUT[ valueA ];
            valueB = LUT[ valueB ];
            valueC = LUT[ valueC ];
            valueD = LUT[ valueD ];
            valueE = LUT[ valueE ];
            valueF = LUT[ valueF ];
            
            uint64_t longResult0 = ((int64_t)value0 << 56) | ((int64_t)value1 << 48) | ((int64_t)value2 << 40) | ((int64_t)value3 << 32)
                                | ((int64_t)value4 << 24) | ((int64_t)value5 << 16) | ((int64_t)value6 << 8) | value7;
            uint64_t longResult1 = ((int64_t)value8 << 56) | ((int64_t)value9 << 48) | ((int64_t)valueA << 40) | ((int64_t)valueB << 32)
                                | ((int64_t)valueC << 24) | ((int64_t)valueD << 16) | ((int64_t)valueE << 8) | valueF;
            
            *( (uint64_t *)(result + j + 0) ) = longResult0;
            *( (uint64_t *)(result + j + 8) ) = longResult1;
        }
        
        // handle remaining bytes
        for ( ; j < count; ++j) {
            result[j] = LUT[ input[j] ];
        }
    }

    record_result( timer(), label );
    verify_LUT(result,count, label);
}

/******************************************************************************/

// read and write 64 bit values, and unroll 2X
// must be special cased for each data size
// NOTE - on a 32 bit CPU this should generate almost the same code as the previous version (assuming correct optimization)
void test_lut6(const uint16_t* input, uint16_t *result, const int count, const uint16_t* LUT, const char *label) {
    int i, j;

    start_timer();

    for(i = 0; i < iterations; ++i) {
        j = 0;
        
        // align to 64 bit boundary on destination
        for ( ; j < count && ((intptr_t(result) & 7) != 0); ++j) {
            result[j] = LUT[ input[j] ];
        }
        
        // read and write 64 bits at a time, unrolled 2X
        for ( ; j < (count - 15); j += 16) {
            uint64_t longValue0 = *( (uint64_t *)(input + j + 0) );
            uint64_t longValue1 = *( (uint64_t *)(input + j + 8) );
            
            uint64_t value0 = (longValue0 >> 48) & 0xFFFF;
            uint64_t value1 = (longValue0 >> 32) & 0xFFFF;
            uint64_t value2 = (longValue0 >> 16) & 0xFFFF;
            uint64_t value3 = longValue0 & 0xFFFF;
            
            uint64_t value4 = (longValue1 >> 48) & 0xFFFF;
            uint64_t value5 = (longValue1 >> 32) & 0xFFFF;
            uint64_t value6 = (longValue1 >> 16) & 0xFFFF;
            uint64_t value7 = longValue1 & 0xFFFF;
            
            value0 = LUT[ value0 ];
            value1 = LUT[ value1 ];
            value2 = LUT[ value2 ];
            value3 = LUT[ value3 ];
            
            value4 = LUT[ value4 ];
            value5 = LUT[ value5 ];
            value6 = LUT[ value6 ];
            value7 = LUT[ value7 ];
            
            uint64_t longResult0 = (value0 << 48) | (value1 << 32) | (value2 << 16) | value3;
            uint64_t longResult1 = (value4 << 48) | (value5 << 32) | (value6 << 16) | value7;
            
            *( (uint64_t *)(result + j + 0) ) = longResult0;
            *( (uint64_t *)(result + j + 8) ) = longResult1;
        }
        
        // handle remaining bytes
        for ( ; j < count; ++j) {
            result[j] = LUT[ input[j] ];
        }
    }

    record_result( timer(), label );
    verify_LUT(result,count, label);
}

/******************************************************************************/

// read and write 64 bit values, and unroll 2X
// must be special cased for each data size
// NOTE - on a 32 bit CPU this should generate almost the same code as the previous version (assuming correct optimization)
void test_lut6(const int16_t* input, int16_t *result, const int count, const int16_t* LUT, const char *label) {
    int i, j;

    start_timer();

    for(i = 0; i < iterations; ++i) {
        j = 0;
        
        // align to 64 bit boundary on destination
        for ( ; j < count && ((intptr_t(result) & 7) != 0); ++j) {
            result[j] = LUT[ input[j] ];
        }
        
        // read and write 64 bits at a time, unrolled 2X
        for ( ; j < (count - 15); j += 16) {
            uint64_t longValue0 = *( (uint64_t *)(input + j + 0) );
            uint64_t longValue1 = *( (uint64_t *)(input + j + 8) );
            
            int16_t value0 = (longValue0 >> 48) & 0xFFFF;
            int16_t value1 = (longValue0 >> 32) & 0xFFFF;
            int16_t value2 = (longValue0 >> 16) & 0xFFFF;
            int16_t value3 = longValue0 & 0xFFFF;
            
            int16_t value4 = (longValue1 >> 48) & 0xFFFF;
            int16_t value5 = (longValue1 >> 32) & 0xFFFF;
            int16_t value6 = (longValue1 >> 16) & 0xFFFF;
            int16_t value7 = longValue1 & 0xFFFF;
            
            value0 = LUT[ value0 ];
            value1 = LUT[ value1 ];
            value2 = LUT[ value2 ];
            value3 = LUT[ value3 ];
            
            value4 = LUT[ value4 ];
            value5 = LUT[ value5 ];
            value6 = LUT[ value6 ];
            value7 = LUT[ value7 ];
            
            uint64_t longResult0 = ((int64_t)value0 << 48) | ((int64_t)value1 << 32) | ((int64_t)value2 << 16) | value3;
            uint64_t longResult1 = ((int64_t)value4 << 48) | ((int64_t)value5 << 32) | ((int64_t)value6 << 16) | value7;
            
            *( (uint64_t *)(result + j + 0) ) = longResult0;
            *( (uint64_t *)(result + j + 8) ) = longResult1;
        }
        
        // handle remaining bytes
        for ( ; j < count; ++j) {
            result[j] = LUT[ input[j] ];
        }
    }

    record_result( timer(), label );
    verify_LUT(result,count, label);
}

/******************************************************************************/

// unroll 2X
template <typename T>
void test_lut7(const T* input, T *result, const int count, const T* LUT, const char *label) {
    int i, j;

    start_timer();

    for(i = 0; i < iterations; ++i) {
        for (j = 0; j < (count - 1); j += 2) {
            T value0 = input[j+0];
            T value1 = input[j+1];
            
            value0 = LUT[ value0 ];
            value1 = LUT[ value1 ];
            
            result[j+0] = value0;
            result[j+1] = value1;
        }
        for (; j < count; ++j) {
            result[j] = LUT[ input[j] ];
        }
    }

    record_result( timer(), label );
    verify_LUT(result,count, label);
}

/******************************************************************************/

// cache block, to deal with 2 way cache issues
// save results to a temporary buffer (stay in L1 cache), then copy back to main memory
template <typename T>
void test_lut8(const T* input, T *result, const int count, const T* LUT, const char *label) {
    int i, j, k;
    const int block_size = 2048/sizeof(T);
    T tempBuffer[ block_size ];

    start_timer();

    for(i = 0; i < iterations; ++i) {
        for (k = 0; k < count; k += block_size ) {
            size_t end = ((k+block_size) < count) ? block_size : (count-k);
            for (j = 0; j < end; ++j) {
                tempBuffer[j] = LUT[ input[k+j] ];
            }
            for (j = 0; j < end; ++j) {
                result[k+j] = tempBuffer[j];
            }
        }
    }

    record_result( timer(), label );
    verify_LUT(result,count, label);
}

/******************************************************************************/
/******************************************************************************/

int main(int argc, char** argv) {

    // output command for documentation:
    int i;
    for (i = 0; i < argc; ++i)
        printf("%s ", argv[i] );
    printf("\n");

    if (argc > 1) base_iterations = atoi(argv[1]);
    if (argc > 2) init_value = (int32_t) atoi(argv[2]);

    uint8_t myLUT8[ 256 ];
    uint16_t myLUT16[ 65536 ];
    

    fill(myLUT8, myLUT8+256, uint8_t(init_value));
    fill(myLUT16, myLUT16+65536, uint16_t(init_value));

    srand( init_value+42 );
    fill_random<uint8_t *, uint8_t>( inputData8, inputData8+SIZE );
    fill_random<uint16_t *, uint16_t>( inputData16, inputData16+SIZE );


// uint8_t
    iterations = base_iterations;

    test_lut1( inputData8, inputData8, SIZE_SMALL, myLUT8, "uint8_t lookup table1 small inplace");
    test_lut2( inputData8, inputData8, SIZE_SMALL, myLUT8, "uint8_t lookup table2 small inplace");
    test_lut3( inputData8, inputData8, SIZE_SMALL, myLUT8, "uint8_t lookup table3 small inplace");
    test_lut4( inputData8, inputData8, SIZE_SMALL, myLUT8, "uint8_t lookup table4 small inplace");
    test_lut5( inputData8, inputData8, SIZE_SMALL, myLUT8, "uint8_t lookup table5 small inplace");
    test_lut6( inputData8, inputData8, SIZE_SMALL, myLUT8, "uint8_t lookup table6 small inplace");
    test_lut7( inputData8, inputData8, SIZE_SMALL, myLUT8, "uint8_t lookup table7 small inplace");
    test_lut8( inputData8, inputData8, SIZE_SMALL, myLUT8, "uint8_t lookup table8 small inplace");
    
    summarize("uint8_t lookup table small inplace", SIZE_SMALL, iterations, kDontShowGMeans, kDontShowPenalty );
    
    test_lut1( inputData8, resultData8, SIZE_SMALL, myLUT8, "uint8_t lookup table1 small");
    test_lut2( inputData8, resultData8, SIZE_SMALL, myLUT8, "uint8_t lookup table2 small");
    test_lut3( inputData8, resultData8, SIZE_SMALL, myLUT8, "uint8_t lookup table3 small");
    test_lut4( inputData8, resultData8, SIZE_SMALL, myLUT8, "uint8_t lookup table4 small");
    test_lut5( inputData8, resultData8, SIZE_SMALL, myLUT8, "uint8_t lookup table5 small");
    test_lut6( inputData8, resultData8, SIZE_SMALL, myLUT8, "uint8_t lookup table6 small");
    test_lut7( inputData8, resultData8, SIZE_SMALL, myLUT8, "uint8_t lookup table7 small");
    test_lut8( inputData8, resultData8, SIZE_SMALL, myLUT8, "uint8_t lookup table8 small");
    
    summarize("uint8_t lookup table small", SIZE_SMALL, iterations, kDontShowGMeans, kDontShowPenalty );


    iterations = std::max( 1, (int)(((uint64_t)base_iterations * SIZE_SMALL) / SIZE) );
    
    test_lut1( inputData8, inputData8, SIZE, myLUT8, "uint8_t lookup table1 large inplace");
    test_lut2( inputData8, inputData8, SIZE, myLUT8, "uint8_t lookup table2 large inplace");
    test_lut3( inputData8, inputData8, SIZE, myLUT8, "uint8_t lookup table3 large inplace");
    test_lut4( inputData8, inputData8, SIZE, myLUT8, "uint8_t lookup table4 large inplace");
    test_lut5( inputData8, inputData8, SIZE, myLUT8, "uint8_t lookup table5 large inplace");
    test_lut6( inputData8, inputData8, SIZE, myLUT8, "uint8_t lookup table6 large inplace");
    test_lut7( inputData8, inputData8, SIZE, myLUT8, "uint8_t lookup table7 large inplace");
    test_lut8( inputData8, inputData8, SIZE, myLUT8, "uint8_t lookup table8 large inplace");
    
    summarize("uint8_t lookup table large inplace", SIZE, iterations, kDontShowGMeans, kDontShowPenalty );
    
    test_lut1( inputData8, resultData8, SIZE, myLUT8, "uint8_t lookup table1 large");
    test_lut2( inputData8, resultData8, SIZE, myLUT8, "uint8_t lookup table2 large");
    test_lut3( inputData8, resultData8, SIZE, myLUT8, "uint8_t lookup table3 large");
    test_lut4( inputData8, resultData8, SIZE, myLUT8, "uint8_t lookup table4 large");
    test_lut5( inputData8, resultData8, SIZE, myLUT8, "uint8_t lookup table5 large");
    test_lut6( inputData8, resultData8, SIZE, myLUT8, "uint8_t lookup table6 large");
    test_lut7( inputData8, resultData8, SIZE, myLUT8, "uint8_t lookup table7 large");
    test_lut8( inputData8, resultData8, SIZE, myLUT8, "uint8_t lookup table8 large");
    
    summarize("uint8_t lookup table large", SIZE, iterations, kDontShowGMeans, kDontShowPenalty );



// int8_t
    iterations = base_iterations;

    test_lut1( (int8_t*)inputData8, (int8_t*)inputData8, SIZE_SMALL, (int8_t*)(myLUT8+128), "int8_t lookup table1 small inplace");
    test_lut2( (int8_t*)inputData8, (int8_t*)inputData8, SIZE_SMALL, (int8_t*)(myLUT8+128), "int8_t lookup table2 small inplace");
    test_lut3( (int8_t*)inputData8, (int8_t*)inputData8, SIZE_SMALL, (int8_t*)(myLUT8+128), "int8_t lookup table3 small inplace");
    test_lut4( (int8_t*)inputData8, (int8_t*)inputData8, SIZE_SMALL, (int8_t*)(myLUT8+128), "int8_t lookup table4 small inplace");
    test_lut5( (int8_t*)inputData8, (int8_t*)inputData8, SIZE_SMALL, (int8_t*)(myLUT8+128), "int8_t lookup table5 small inplace");
    test_lut6( (int8_t*)inputData8, (int8_t*)inputData8, SIZE_SMALL, (int8_t*)(myLUT8+128), "int8_t lookup table6 small inplace");
    test_lut7( (int8_t*)inputData8, (int8_t*)inputData8, SIZE_SMALL, (int8_t*)(myLUT8+128), "int8_t lookup table7 small inplace");
    test_lut8( (int8_t*)inputData8, (int8_t*)inputData8, SIZE_SMALL, (int8_t*)(myLUT8+128), "int8_t lookup table8 small inplace");
    
    summarize("int8_t lookup table small inplace", SIZE_SMALL, iterations, kDontShowGMeans, kDontShowPenalty );
    
    test_lut1( (int8_t*)inputData8, (int8_t*)resultData8, SIZE_SMALL, (int8_t*)(myLUT8+128), "int8_t lookup table1 small");
    test_lut2( (int8_t*)inputData8, (int8_t*)resultData8, SIZE_SMALL, (int8_t*)(myLUT8+128), "int8_t lookup table2 small");
    test_lut3( (int8_t*)inputData8, (int8_t*)resultData8, SIZE_SMALL, (int8_t*)(myLUT8+128), "int8_t lookup table3 small");
    test_lut4( (int8_t*)inputData8, (int8_t*)resultData8, SIZE_SMALL, (int8_t*)(myLUT8+128), "int8_t lookup table4 small");
    test_lut5( (int8_t*)inputData8, (int8_t*)resultData8, SIZE_SMALL, (int8_t*)(myLUT8+128), "int8_t lookup table5 small");
    test_lut6( (int8_t*)inputData8, (int8_t*)resultData8, SIZE_SMALL, (int8_t*)(myLUT8+128), "int8_t lookup table6 small");
    test_lut7( (int8_t*)inputData8, (int8_t*)resultData8, SIZE_SMALL, (int8_t*)(myLUT8+128), "int8_t lookup table7 small");
    test_lut8( (int8_t*)inputData8, (int8_t*)resultData8, SIZE_SMALL, (int8_t*)(myLUT8+128), "int8_t lookup table8 small");
    
    summarize("int8_t lookup table small", SIZE_SMALL, iterations, kDontShowGMeans, kDontShowPenalty );


    iterations = std::max( 1, (int)(((uint64_t)base_iterations * SIZE_SMALL) / SIZE) );
    
    test_lut1( (int8_t*)inputData8, (int8_t*)inputData8, SIZE, (int8_t*)(myLUT8+128), "int8_t lookup table1 large inplace");
    test_lut2( (int8_t*)inputData8, (int8_t*)inputData8, SIZE, (int8_t*)(myLUT8+128), "int8_t lookup table2 large inplace");
    test_lut3( (int8_t*)inputData8, (int8_t*)inputData8, SIZE, (int8_t*)(myLUT8+128), "int8_t lookup table3 large inplace");
    test_lut4( (int8_t*)inputData8, (int8_t*)inputData8, SIZE, (int8_t*)(myLUT8+128), "int8_t lookup table4 large inplace");
    test_lut5( (int8_t*)inputData8, (int8_t*)inputData8, SIZE, (int8_t*)(myLUT8+128), "int8_t lookup table5 large inplace");
    test_lut6( (int8_t*)inputData8, (int8_t*)inputData8, SIZE, (int8_t*)(myLUT8+128), "int8_t lookup table6 large inplace");
    test_lut7( (int8_t*)inputData8, (int8_t*)inputData8, SIZE, (int8_t*)(myLUT8+128), "int8_t lookup table7 large inplace");
    test_lut8( (int8_t*)inputData8, (int8_t*)inputData8, SIZE, (int8_t*)(myLUT8+128), "int8_t lookup table8 large inplace");
    
    summarize("int8_t lookup table large inplace", SIZE, iterations, kDontShowGMeans, kDontShowPenalty );
    
    test_lut1( (int8_t*)inputData8, (int8_t*)resultData8, SIZE, (int8_t*)(myLUT8+128), "int8_t lookup table1 large");
    test_lut2( (int8_t*)inputData8, (int8_t*)resultData8, SIZE, (int8_t*)(myLUT8+128), "int8_t lookup table2 large");
    test_lut3( (int8_t*)inputData8, (int8_t*)resultData8, SIZE, (int8_t*)(myLUT8+128), "int8_t lookup table3 large");
    test_lut4( (int8_t*)inputData8, (int8_t*)resultData8, SIZE, (int8_t*)(myLUT8+128), "int8_t lookup table4 large");
    test_lut5( (int8_t*)inputData8, (int8_t*)resultData8, SIZE, (int8_t*)(myLUT8+128), "int8_t lookup table5 large");
    test_lut6( (int8_t*)inputData8, (int8_t*)resultData8, SIZE, (int8_t*)(myLUT8+128), "int8_t lookup table6 large");
    test_lut7( (int8_t*)inputData8, (int8_t*)resultData8, SIZE, (int8_t*)(myLUT8+128), "int8_t lookup table7 large");
    test_lut8( (int8_t*)inputData8, (int8_t*)resultData8, SIZE, (int8_t*)(myLUT8+128), "int8_t lookup table8 large");
    
    summarize("int8_t lookup table large", SIZE, iterations, kDontShowGMeans, kDontShowPenalty );



// uint16_t
    iterations = base_iterations;

    test_lut1( inputData16, inputData16, SIZE_SMALL, myLUT16, "uint16_t lookup table1 small inplace");
    test_lut2( inputData16, inputData16, SIZE_SMALL, myLUT16, "uint16_t lookup table2 small inplace");
    test_lut3( inputData16, inputData16, SIZE_SMALL, myLUT16, "uint16_t lookup table3 small inplace");
    test_lut4( inputData16, inputData16, SIZE_SMALL, myLUT16, "uint16_t lookup table4 small inplace");
    test_lut5( inputData16, inputData16, SIZE_SMALL, myLUT16, "uint16_t lookup table5 small inplace");
    test_lut6( inputData16, inputData16, SIZE_SMALL, myLUT16, "uint16_t lookup table6 small inplace");
    test_lut7( inputData16, inputData16, SIZE_SMALL, myLUT16, "uint16_t lookup table7 small inplace");
    test_lut8( inputData16, inputData16, SIZE_SMALL, myLUT16, "uint16_t lookup table8 small inplace");
    
    summarize("uint16_t lookup table small inplace", SIZE_SMALL, iterations, kDontShowGMeans, kDontShowPenalty );
    
    test_lut1( inputData16, resultData16, SIZE_SMALL, myLUT16, "uint16_t lookup table1 small");
    test_lut2( inputData16, resultData16, SIZE_SMALL, myLUT16, "uint16_t lookup table2 small");
    test_lut3( inputData16, resultData16, SIZE_SMALL, myLUT16, "uint16_t lookup table3 small");
    test_lut4( inputData16, resultData16, SIZE_SMALL, myLUT16, "uint16_t lookup table4 small");
    test_lut5( inputData16, resultData16, SIZE_SMALL, myLUT16, "uint16_t lookup table5 small");
    test_lut6( inputData16, resultData16, SIZE_SMALL, myLUT16, "uint16_t lookup table6 small");
    test_lut7( inputData16, resultData16, SIZE_SMALL, myLUT16, "uint16_t lookup table7 small");
    test_lut8( inputData16, resultData16, SIZE_SMALL, myLUT16, "uint16_t lookup table8 small");
    
    summarize("uint16_t lookup table small", SIZE_SMALL, iterations, kDontShowGMeans, kDontShowPenalty );


    iterations = std::max( 1, (int)(((uint64_t)base_iterations * SIZE_SMALL) / SIZE) );
    
    test_lut1( inputData16, inputData16, SIZE, myLUT16, "uint16_t lookup table1 large inplace");
    test_lut2( inputData16, inputData16, SIZE, myLUT16, "uint16_t lookup table2 large inplace");
    test_lut3( inputData16, inputData16, SIZE, myLUT16, "uint16_t lookup table3 large inplace");
    test_lut4( inputData16, inputData16, SIZE, myLUT16, "uint16_t lookup table4 large inplace");
    test_lut5( inputData16, inputData16, SIZE, myLUT16, "uint16_t lookup table5 large inplace");
    test_lut6( inputData16, inputData16, SIZE, myLUT16, "uint16_t lookup table6 large inplace");
    test_lut7( inputData16, inputData16, SIZE, myLUT16, "uint16_t lookup table7 large inplace");
    test_lut8( inputData16, inputData16, SIZE, myLUT16, "uint16_t lookup table8 large inplace");
    
    summarize("uint16_t lookup table large inplace", SIZE, iterations, kDontShowGMeans, kDontShowPenalty );
    
    test_lut1( inputData16, resultData16, SIZE, myLUT16, "uint16_t lookup table1 large");
    test_lut2( inputData16, resultData16, SIZE, myLUT16, "uint16_t lookup table2 large");
    test_lut3( inputData16, resultData16, SIZE, myLUT16, "uint16_t lookup table3 large");
    test_lut4( inputData16, resultData16, SIZE, myLUT16, "uint16_t lookup table4 large");
    test_lut5( inputData16, resultData16, SIZE, myLUT16, "uint16_t lookup table5 large");
    test_lut6( inputData16, resultData16, SIZE, myLUT16, "uint16_t lookup table6 large");
    test_lut7( inputData16, resultData16, SIZE, myLUT16, "uint16_t lookup table7 large");
    test_lut8( inputData16, resultData16, SIZE, myLUT16, "uint16_t lookup table8 large");
    
    summarize("uint16_t lookup table large", SIZE, iterations, kDontShowGMeans, kDontShowPenalty );


// int16_t
    iterations = base_iterations;

    test_lut1( (int16_t*)inputData16, (int16_t*)inputData16, SIZE_SMALL, (int16_t*)(myLUT16+32768), "int16_t lookup table1 small inplace");
    test_lut2( (int16_t*)inputData16, (int16_t*)inputData16, SIZE_SMALL, (int16_t*)(myLUT16+32768), "int16_t lookup table2 small inplace");
    test_lut3( (int16_t*)inputData16, (int16_t*)inputData16, SIZE_SMALL, (int16_t*)(myLUT16+32768), "int16_t lookup table3 small inplace");
    test_lut4( (int16_t*)inputData16, (int16_t*)inputData16, SIZE_SMALL, (int16_t*)(myLUT16+32768), "int16_t lookup table4 small inplace");
    test_lut5( (int16_t*)inputData16, (int16_t*)inputData16, SIZE_SMALL, (int16_t*)(myLUT16+32768), "int16_t lookup table5 small inplace");
    test_lut6( (int16_t*)inputData16, (int16_t*)inputData16, SIZE_SMALL, (int16_t*)(myLUT16+32768), "int16_t lookup table6 small inplace");
    test_lut7( (int16_t*)inputData16, (int16_t*)inputData16, SIZE_SMALL, (int16_t*)(myLUT16+32768), "int16_t lookup table7 small inplace");
    test_lut8( (int16_t*)inputData16, (int16_t*)inputData16, SIZE_SMALL, (int16_t*)(myLUT16+32768), "int16_t lookup table8 small inplace");
    
    summarize("int16_t lookup table small inplace", SIZE_SMALL, iterations, kDontShowGMeans, kDontShowPenalty );
    
    test_lut1( (int16_t*)inputData16, (int16_t*)resultData16, SIZE_SMALL, (int16_t*)(myLUT16+32768), "int16_t lookup table1 small");
    test_lut2( (int16_t*)inputData16, (int16_t*)resultData16, SIZE_SMALL, (int16_t*)(myLUT16+32768), "int16_t lookup table2 small");
    test_lut3( (int16_t*)inputData16, (int16_t*)resultData16, SIZE_SMALL, (int16_t*)(myLUT16+32768), "int16_t lookup table3 small");
    test_lut4( (int16_t*)inputData16, (int16_t*)resultData16, SIZE_SMALL, (int16_t*)(myLUT16+32768), "int16_t lookup table4 small");
    test_lut5( (int16_t*)inputData16, (int16_t*)resultData16, SIZE_SMALL, (int16_t*)(myLUT16+32768), "int16_t lookup table5 small");
    test_lut6( (int16_t*)inputData16, (int16_t*)resultData16, SIZE_SMALL, (int16_t*)(myLUT16+32768), "int16_t lookup table6 small");
    test_lut7( (int16_t*)inputData16, (int16_t*)resultData16, SIZE_SMALL, (int16_t*)(myLUT16+32768), "int16_t lookup table7 small");
    test_lut8( (int16_t*)inputData16, (int16_t*)resultData16, SIZE_SMALL, (int16_t*)(myLUT16+32768), "int16_t lookup table8 small");
    
    summarize("int16_t lookup table small", SIZE_SMALL, iterations, kDontShowGMeans, kDontShowPenalty );


    iterations = std::max( 1, (int)(((uint64_t)base_iterations * SIZE_SMALL) / SIZE) );
    
    test_lut1( (int16_t*)inputData16, (int16_t*)inputData16, SIZE, (int16_t*)(myLUT16+32768), "int16_t lookup table1 large inplace");
    test_lut2( (int16_t*)inputData16, (int16_t*)inputData16, SIZE, (int16_t*)(myLUT16+32768), "int16_t lookup table2 large inplace");
    test_lut3( (int16_t*)inputData16, (int16_t*)inputData16, SIZE, (int16_t*)(myLUT16+32768), "int16_t lookup table3 large inplace");
    test_lut4( (int16_t*)inputData16, (int16_t*)inputData16, SIZE, (int16_t*)(myLUT16+32768), "int16_t lookup table4 large inplace");
    test_lut5( (int16_t*)inputData16, (int16_t*)inputData16, SIZE, (int16_t*)(myLUT16+32768), "int16_t lookup table5 large inplace");
    test_lut6( (int16_t*)inputData16, (int16_t*)inputData16, SIZE, (int16_t*)(myLUT16+32768), "int16_t lookup table6 large inplace");
    test_lut7( (int16_t*)inputData16, (int16_t*)inputData16, SIZE, (int16_t*)(myLUT16+32768), "int16_t lookup table7 large inplace");
    test_lut8( (int16_t*)inputData16, (int16_t*)inputData16, SIZE, (int16_t*)(myLUT16+32768), "int16_t lookup table8 large inplace");
    
    summarize("int16_t lookup table large inplace", SIZE, iterations, kDontShowGMeans, kDontShowPenalty );
    
    test_lut1( (int16_t*)inputData16, (int16_t*)resultData16, SIZE, (int16_t*)(myLUT16+32768), "int16_t lookup table1 large");
    test_lut2( (int16_t*)inputData16, (int16_t*)resultData16, SIZE, (int16_t*)(myLUT16+32768), "int16_t lookup table2 large");
    test_lut3( (int16_t*)inputData16, (int16_t*)resultData16, SIZE, (int16_t*)(myLUT16+32768), "int16_t lookup table3 large");
    test_lut4( (int16_t*)inputData16, (int16_t*)resultData16, SIZE, (int16_t*)(myLUT16+32768), "int16_t lookup table4 large");
    test_lut5( (int16_t*)inputData16, (int16_t*)resultData16, SIZE, (int16_t*)(myLUT16+32768), "int16_t lookup table5 large");
    test_lut6( (int16_t*)inputData16, (int16_t*)resultData16, SIZE, (int16_t*)(myLUT16+32768), "int16_t lookup table6 large");
    test_lut7( (int16_t*)inputData16, (int16_t*)resultData16, SIZE, (int16_t*)(myLUT16+32768), "int16_t lookup table7 large");
    test_lut8( (int16_t*)inputData16, (int16_t*)resultData16, SIZE, (int16_t*)(myLUT16+32768), "int16_t lookup table8 large");
    
    summarize("int16_t lookup table large", SIZE, iterations, kDontShowGMeans, kDontShowPenalty );



    return 0;
}

// the end
/******************************************************************************/
/******************************************************************************/
