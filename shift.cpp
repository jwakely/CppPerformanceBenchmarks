/*
    Copyright 2007-2008 Adobe Systems Incorporated
    Copyright 2018 Chris Cox
    Distributed under the MIT License (see accompanying file LICENSE_1_0_0.txt
    or a copy at http://stlab.adobe.com/licenses.html )


Goal: Test compiler optimizations related to the shift operators.


Assumptions:
    
    1) The compiler will recognize and remove identity/zero shifts.
        value >> 0        ==>        value
        value << 0        ==>        value

    2) The compiler will collapse sequential shifts of the same direction into a single shift.
        (((((value >> A) >> B) >> C) >> D) >> E)    ==>   (value >> (A+B+C+D+E))
        (((((value << A) << B) << C) << D) << E)    ==>   (value << (A+B+C+D+E))
    
    3) The compiler will collapse consecutive right and left shifts of equal amount into a mask operation.
        ((value >> K) << K)  ==>    (value & ~((1<<K)-1))
    
    4) The compiler will collapse consecutive left and right shifts of equal amount on an unsigned value into a mask operation.
        (((uint)value << K) >> K)  ==>    (value & (-1 >> K))



TODO:
    Not sure if these are easily testable - still a single instruction, fast op
    1) the compiler can special case 64 bit value shifts by 32 bits
        (input[i] >> 32)
        ((uint64_t)inputA[i] << 32) | inputB[i];
    2) the compiler can special case 32 bit value shifts by 16 bits
    3) the compiler can special case 16 bit value shifts by 8 bits

*/

#include "benchmark_stdint.hpp"
#include <stddef.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include "benchmark_results.h"
#include "benchmark_timer.h"

/******************************************************************************/

// this constant may need to be adjusted to give reasonable minimum times
// For best results, times should be about 1.0 seconds for the minimum test run
int iterations = 6000000;


// 8000 items, or between 8k and 64k of data
// this is intended to remain within the L2 cache of most common CPUs
#define SIZE     8000


// initial value for filling our arrays, may be changed from the command line
uint64_t init_value = 0x0102030405060708ULL;

/******************************************************************************/

#include "benchmark_shared_tests.h"

/******************************************************************************/

// our global arrays of numbers to be operated upon

uint64_t data64unsigned[SIZE];
int64_t data64[SIZE];

uint32_t data32unsigned[SIZE];
int32_t data32[SIZE];

uint16_t data16unsigned[SIZE];
int16_t data16[SIZE];

uint8_t data8unsigned[SIZE];
int8_t data8[SIZE];

/******************************************************************************/
/******************************************************************************/

template <typename T>
    struct shift_right_constant {
      static T do_shift(T input) { return (input >> 5); }
    };

/******************************************************************************/

template <typename T>
    struct shift_right_repeated_constant {
      static T do_shift(T input) { return (((((input >> 1) >> 1) >> 1) >> 1) >> 1); }
    };

/******************************************************************************/

template <typename T>
    struct shift_left_constant {
      static T do_shift(T input) { return (input << 5); }
    };

/******************************************************************************/

template <typename T>
    struct shift_left_repeated_constant {
      static T do_shift(T input) { return (((((input << 1) << 1) << 1) << 1) << 1); }
    };

/******************************************************************************/

template <typename T>
    struct shift_right_variable {
      static T do_shift(T input, const int shift) { return (input >> shift); }
    };

/******************************************************************************/

template <typename T>
    struct shift_right_repeated_variable {
      static T do_shift(T input, const int shift) { return (((((input >> shift) >> shift) >> shift) >> shift) >> shift); }
    };

/******************************************************************************/

template <typename T>
    struct shift_left_variable {
      static T do_shift(T input, const int shift) { return (input << shift); }
    };

/******************************************************************************/

template <typename T>
    struct shift_left_repeated_variable {
      static T do_shift(T input, const int shift) { return (((((input << shift) << shift) << shift) << shift) << shift); }
    };

/******************************************************************************/

template <typename T>
    struct mask_low_by_shift_constant {
      static T do_shift(T input) { return ((input >> 4) << 4); }
    };

/******************************************************************************/

template <typename T>
    struct mask_low_constant {
      static T do_shift(T input) { return (input & ~ ((T)15) ); }
    };

/******************************************************************************/

template <typename T>
    struct mask_low_by_shift_variable {
      static T do_shift(T input, const int shift) { return ((input >> shift) << shift); }
    };

/******************************************************************************/

template <typename T>
    struct mask_low_variable {
      static T do_shift(T input, const int shift) { return (input & ~ (((T)(1LL << shift) - 1)) ); }
    };

/******************************************************************************/

template <typename T>
    struct mask_high_by_shift_constant {
      static T do_shift(T input) { return ((input << 4) >> 4); }
    };

/******************************************************************************/

template <typename T>
    struct mask_high_constant {
      static T do_shift(T input) { return (input & ((~(T)0)>>4) ); }
    };

/******************************************************************************/

template <typename T>
    struct mask_high_by_shift_variable {
      static T do_shift(T input, const int shift) { return ((input << shift) >> shift); }
    };

/******************************************************************************/

template <typename T>
    struct mask_high_variable {
      static T do_shift(T input, const int shift) { return (input & ((~(T)0)>>shift) ); }
    };

/******************************************************************************/

template <typename T>
    struct shift_identity{
      static T do_shift(T input) { return (input); }
    };

/******************************************************************************/

template <typename T>
    struct shift_right_zero {
      static T do_shift(T input) { return (input >> 0); }
    };

/******************************************************************************/

template <typename T>
    struct shift_left_zero {
      static T do_shift(T input) { return (input << 0); }
    };

/******************************************************************************/
/******************************************************************************/

int main(int argc, char** argv) {
    int shift_factor = 1;
  
    // output command for documentation:
    int i;
    for (i = 0; i < argc; ++i)
        printf("%s ", argv[i] );
    printf("\n");

    if (argc > 1) iterations = atoi(argv[1]);
    if (argc > 2) init_value = (long) atoi(argv[2]);
    if (argc > 3) shift_factor = (unsigned int) atoi(argv[3]);



// verify the operations    (grumble, grumble compiler bugs)
    const uint8_t testPattern8 = 0%01010101;
    const uint8_t shiftedPatternR8 = testPattern8 >> 5;
    const uint8_t shiftedPatternL8 = testPattern8 << 5;
    const uint8_t shiftedPatternMLow8 = testPattern8 & 0xF0;
    const uint8_t shiftedPatternMHigh8 = testPattern8 & 0x0F;

    const uint16_t testPattern16 = 0x0102;
    const uint16_t shiftedPatternR16 = testPattern16 >> 5;
    const uint16_t shiftedPatternL16 = testPattern16 << 5;
    const uint16_t shiftedPatternMLow16 = testPattern16 & 0xFFF0;
    const uint16_t shiftedPatternMHigh16 = testPattern16 & 0x0FFF;
    
    const uint32_t testPattern32 = 0x01020304UL;
    const uint32_t shiftedPatternR32 = testPattern32 >> 5;
    const uint32_t shiftedPatternL32 = testPattern32 << 5;
    const uint32_t shiftedPatternMLow32 = testPattern32 & 0xFFFFFFF0UL;
    const uint32_t shiftedPatternMHigh32 = testPattern32 & 0x0FFFFFFFUL;
    
    const uint64_t testPattern64 = 0x5152535455565758ULL;
    const uint64_t shiftedPatternR64 = testPattern64 >> 5;
    const uint64_t shiftedPatternL64 = testPattern64 << 5;
    const uint64_t shiftedPatternMLow64 = testPattern64 & 0xFFFFFFFFFFFFFFF0ULL;
    const uint64_t shiftedPatternMHigh64 = testPattern64 & 0x0FFFFFFFFFFFFFFFULL;


    // uint8_t
    if ( shiftedPatternR8 != shift_right_constant<uint8_t>::do_shift(testPattern8) )
        printf("shift_right_constant got incorrect results\n");
    if ( shiftedPatternR8 != shift_right_repeated_constant<uint8_t>::do_shift(testPattern8) )
        printf("shift_right_repeated_constant got incorrect results\n");
    if ( shiftedPatternL8 != shift_left_constant<uint8_t>::do_shift(testPattern8) )
        printf("shift_left_constant got incorrect results\n");
    if ( shiftedPatternL8 != shift_left_repeated_constant<uint8_t>::do_shift(testPattern8) )
        printf("shift_left_repeated_constant got incorrect results\n");
    if ( testPattern8 != shift_right_zero<uint8_t>::do_shift(testPattern8) )
        printf("shift_right_zero got incorrect results\n");
    if ( testPattern8 != shift_left_zero<uint8_t>::do_shift(testPattern8) )
        printf("shift_left_zero got incorrect results\n");
    if ( shiftedPatternR8 != shift_right_variable<uint8_t>::do_shift(testPattern8,5) )
        printf("shift_right_variable got incorrect results\n");
    if ( shiftedPatternR8 != shift_right_repeated_variable<uint8_t>::do_shift(testPattern8,1) )
        printf("shift_right_repeated_variable got incorrect results\n");
    if ( shiftedPatternL8 != shift_left_variable<uint8_t>::do_shift(testPattern8,5) )
        printf("shift_left_variable got incorrect results\n");
    if ( shiftedPatternL8 != shift_left_repeated_variable<uint8_t>::do_shift(testPattern8,1) )
        printf("shift_left_repeated_variable got incorrect results\n");
    if ( shiftedPatternMLow8 != mask_low_constant<uint8_t>::do_shift(testPattern8) )
        printf("mask_low_constant got incorrect results\n");
    if ( shiftedPatternMLow8 != mask_low_by_shift_constant<uint8_t>::do_shift(testPattern8) )
        printf("mask_low_by_shift_constant got incorrect results\n");
    if ( shiftedPatternMLow8 != mask_low_variable<uint8_t>::do_shift(testPattern8,4) )
        printf("mask_low_variable got incorrect results\n");
    if ( shiftedPatternMLow8 != mask_low_by_shift_variable<uint8_t>::do_shift(testPattern8,4) )
        printf("mask_low_by_shift_variable got incorrect results\n");
    if ( shiftedPatternMHigh8 != mask_high_constant<uint8_t>::do_shift(testPattern8) )
        printf("mask_high_constant got incorrect results\n");
    if ( shiftedPatternMHigh8 != mask_high_by_shift_constant<uint8_t>::do_shift(testPattern8) )
        printf("mask_high_by_shift_constant got incorrect results\n");
    if ( shiftedPatternMHigh8 != mask_high_variable<uint8_t>::do_shift(testPattern8,4) )
        printf("mask_high_variable got incorrect results\n");
    if ( shiftedPatternMHigh8 != mask_high_by_shift_variable<uint8_t>::do_shift(testPattern8,4) )
        printf("mask_high_by_shift_variable got incorrect results\n");
    
    
    // uint16_t
    if ( shiftedPatternR16 != shift_right_constant<uint16_t>::do_shift(testPattern16) )
        printf("shift_right_constant got incorrect results\n");
    if ( shiftedPatternR16 != shift_right_repeated_constant<uint16_t>::do_shift(testPattern16) )
        printf("shift_right_repeated_constant got incorrect results\n");
    if ( shiftedPatternL16 != shift_left_constant<uint16_t>::do_shift(testPattern16) )
        printf("shift_left_constant got incorrect results\n");
    if ( shiftedPatternL16 != shift_left_repeated_constant<uint16_t>::do_shift(testPattern16) )
        printf("shift_left_repeated_constant got incorrect results\n");
    if ( testPattern16 != shift_right_zero<uint16_t>::do_shift(testPattern16) )
        printf("shift_right_zero got incorrect results\n");
    if ( testPattern16 != shift_left_zero<uint16_t>::do_shift(testPattern16) )
        printf("shift_left_zero got incorrect results\n");
    if ( shiftedPatternR16 != shift_right_variable<uint16_t>::do_shift(testPattern16,5) )
        printf("shift_right_variable got incorrect results\n");
    if ( shiftedPatternR16 != shift_right_repeated_variable<uint16_t>::do_shift(testPattern16,1) )
        printf("shift_right_repeated_variable got incorrect results\n");
    if ( shiftedPatternL16 != shift_left_variable<uint16_t>::do_shift(testPattern16,5) )
        printf("shift_left_variable got incorrect results\n");
    if ( shiftedPatternL16 != shift_left_repeated_variable<uint16_t>::do_shift(testPattern16,1) )
        printf("shift_left_repeated_variable got incorrect results\n");
    if ( shiftedPatternMLow16 != mask_low_constant<uint16_t>::do_shift(testPattern16) )
        printf("mask_low_constant got incorrect results\n");
    if ( shiftedPatternMLow16 != mask_low_by_shift_constant<uint16_t>::do_shift(testPattern16) )
        printf("mask_low_by_shift_constant got incorrect results\n");
    if ( shiftedPatternMLow16 != mask_low_variable<uint16_t>::do_shift(testPattern16,4) )
        printf("mask_low_variable got incorrect results\n");
    if ( shiftedPatternMLow16 != mask_low_by_shift_variable<uint16_t>::do_shift(testPattern16,4) )
        printf("mask_low_by_shift_variable got incorrect results\n");
    if ( shiftedPatternMHigh16 != mask_high_constant<uint16_t>::do_shift(testPattern16) )
        printf("mask_high_constant got incorrect results\n");
    if ( shiftedPatternMHigh16 != mask_high_by_shift_constant<uint16_t>::do_shift(testPattern16) )
        printf("mask_high_by_shift_constant got incorrect results\n");
    if ( shiftedPatternMHigh16 != mask_high_variable<uint16_t>::do_shift(testPattern16,4) )
        printf("mask_high_variable got incorrect results\n");
    if ( shiftedPatternMHigh16 != mask_high_by_shift_variable<uint16_t>::do_shift(testPattern16,4) )
        printf("mask_high_by_shift_variable got incorrect results\n");
    
    
    // uint32_t
    if ( shiftedPatternR32 != shift_right_constant<uint32_t>::do_shift(testPattern32) )
        printf("shift_right_constant got incorrect results\n");
    if ( shiftedPatternR32 != shift_right_repeated_constant<uint32_t>::do_shift(testPattern32) )
        printf("shift_right_repeated_constant got incorrect results\n");
    if ( shiftedPatternL32 != shift_left_constant<uint32_t>::do_shift(testPattern32) )
        printf("shift_left_constant got incorrect results\n");
    if ( shiftedPatternL32 != shift_left_repeated_constant<uint32_t>::do_shift(testPattern32) )
        printf("shift_left_repeated_constant got incorrect results\n");
    if ( testPattern32 != shift_right_zero<uint32_t>::do_shift(testPattern32) )
        printf("shift_right_zero got incorrect results\n");
    if ( testPattern32 != shift_left_zero<uint32_t>::do_shift(testPattern32) )
        printf("shift_left_zero got incorrect results\n");
    if ( shiftedPatternR32 != shift_right_variable<uint32_t>::do_shift(testPattern32,5) )
        printf("shift_right_variable got incorrect results\n");
    if ( shiftedPatternR32 != shift_right_repeated_variable<uint32_t>::do_shift(testPattern32,1) )
        printf("shift_right_repeated_variable got incorrect results\n");
    if ( shiftedPatternL32 != shift_left_variable<uint32_t>::do_shift(testPattern32,5) )
        printf("shift_left_variable got incorrect results\n");
    if ( shiftedPatternL32 != shift_left_repeated_variable<uint32_t>::do_shift(testPattern32,1) )
        printf("shift_left_repeated_variable got incorrect results\n");
    if ( shiftedPatternMLow32 != mask_low_constant<uint32_t>::do_shift(testPattern32))
        printf("mask_low_constant got incorrect results\n");
    if ( shiftedPatternMLow32 != mask_low_by_shift_constant<uint32_t>::do_shift(testPattern32))
        printf("mask_low_by_shift_constant got incorrect results\n");
    if ( shiftedPatternMLow32 != mask_low_variable<uint32_t>::do_shift(testPattern32,4) )
        printf("mask_low_variable got incorrect results\n");
    if ( shiftedPatternMLow32 != mask_low_by_shift_variable<uint32_t>::do_shift(testPattern32,4) )
        printf("mask_low_by_shift_variable got incorrect results\n");
    if ( shiftedPatternMHigh32 != mask_high_constant<uint32_t>::do_shift(testPattern32) )
        printf("mask_high_constant got incorrect results\n");
    if ( shiftedPatternMHigh32 != mask_high_by_shift_constant<uint32_t>::do_shift(testPattern32) )
        printf("mask_high_by_shift_constant got incorrect results\n");
    if ( shiftedPatternMHigh32 != mask_high_variable<uint32_t>::do_shift(testPattern32,4) )
        printf("mask_high_variable got incorrect results\n");
    if ( shiftedPatternMHigh32 != mask_high_by_shift_variable<uint32_t>::do_shift(testPattern32,4) )
        printf("mask_high_by_shift_variable got incorrect results\n");
    
    
    // uint64_t
    if ( shiftedPatternR64 != shift_right_constant<uint64_t>::do_shift(testPattern64) )
        printf("shift_right_constant got incorrect results\n");
    if ( shiftedPatternR64 != shift_right_repeated_constant<uint64_t>::do_shift(testPattern64) )
        printf("shift_right_repeated_constant got incorrect results\n");
    if ( shiftedPatternL64 != shift_left_constant<uint64_t>::do_shift(testPattern64) )
        printf("shift_left_constant got incorrect results\n");
    if ( shiftedPatternL64 != shift_left_repeated_constant<uint64_t>::do_shift(testPattern64) )
        printf("shift_left_repeated_constant got incorrect results\n");
    if ( testPattern64 != shift_right_zero<uint64_t>::do_shift(testPattern64) )
        printf("shift_right_zero got incorrect results\n");
    if ( testPattern64 != shift_left_zero<uint64_t>::do_shift(testPattern64) )
        printf("shift_left_zero got incorrect results\n");
    if ( shiftedPatternR64 != shift_right_variable<uint64_t>::do_shift(testPattern64,5) )
        printf("shift_right_variable got incorrect results\n");
    if ( shiftedPatternR64 != shift_right_repeated_variable<uint64_t>::do_shift(testPattern64,1) )
        printf("shift_right_repeated_variable got incorrect results\n");
    if ( shiftedPatternL64 != shift_left_variable<uint64_t>::do_shift(testPattern64,5) )
        printf("shift_left_variable got incorrect results\n");
    if ( shiftedPatternL64 != shift_left_repeated_variable<uint64_t>::do_shift(testPattern64,1) )
        printf("shift_left_repeated_variable got incorrect results\n");
    if ( shiftedPatternMLow64 != mask_low_constant<uint64_t>::do_shift(testPattern64))
        printf("mask_low_constant got incorrect results\n");
    if ( shiftedPatternMLow64 != mask_low_by_shift_constant<uint64_t>::do_shift(testPattern64))
        printf("mask_low_by_shift_constant got incorrect results\n");
    if ( shiftedPatternMLow64 != mask_low_variable<uint64_t>::do_shift(testPattern64,4) )
        printf("mask_low_variable got incorrect results\n");
    if ( shiftedPatternMLow64 != mask_low_by_shift_variable<uint64_t>::do_shift(testPattern64,4) )
        printf("mask_low_by_shift_variable got incorrect results\n");
    if ( shiftedPatternMHigh64 != mask_high_constant<uint64_t>::do_shift(testPattern64) )
        printf("mask_high_constant got incorrect results\n");
    if ( shiftedPatternMHigh64 != mask_high_by_shift_constant<uint64_t>::do_shift(testPattern64) )
        printf("mask_high_by_shift_constant got incorrect results\n");
    if ( shiftedPatternMHigh64 != mask_high_variable<uint64_t>::do_shift(testPattern64,4) )
        printf("mask_high_variable got incorrect results\n");
    if ( shiftedPatternMHigh64 != mask_high_by_shift_variable<uint64_t>::do_shift(testPattern64,4) )
        printf("mask_high_by_shift_variable got incorrect results\n");

    
    
    

    // instruction combining multiple constant shifts

    // unsigned 8 bit
    fill(data8unsigned, data8unsigned+SIZE, uint8_t(init_value));
    test_constant<uint8_t, shift_right_constant<uint8_t> >(data8unsigned, SIZE, "uint8_t constant right shift");
    test_constant<uint8_t, shift_right_repeated_constant<uint8_t> >(data8unsigned, SIZE, "uint8_t repeated constant right shift");
    
    test_constant<uint8_t, shift_left_constant<uint8_t> >(data8unsigned, SIZE, "uint8_t constant left shift");
    test_constant<uint8_t, shift_left_repeated_constant<uint8_t> >(data8unsigned, SIZE, "uint8_t repeated constant left shift");
    
    test_constant<uint8_t, shift_identity<uint8_t> >(data8unsigned, SIZE, "uint8_t identity");
    test_constant<uint8_t, shift_right_zero<uint8_t> >(data8unsigned, SIZE, "uint8_t right shift zero");
    test_constant<uint8_t, shift_left_zero<uint8_t> >(data8unsigned, SIZE, "uint8_t left shift zero");
    
    // signed 8 bit
    fill(data8, data8+SIZE, int8_t(init_value));
    test_constant<int8_t, shift_right_constant<int8_t> >(data8, SIZE, "int8_t constant right shift");
    test_constant<int8_t, shift_right_repeated_constant<int8_t> >(data8, SIZE, "int8_t repeated constant right shift");
    
    test_constant<int8_t, shift_left_constant<int8_t> >(data8, SIZE, "int8_t constant left shift");
    test_constant<int8_t, shift_left_repeated_constant<int8_t> >(data8, SIZE, "int8_t repeated constant left shift");
    
    test_constant<int8_t, shift_identity<int8_t> >(data8, SIZE, "int8_t identity");
    test_constant<int8_t, shift_right_zero<int8_t> >(data8, SIZE, "int8_t right shift zero");
    test_constant<int8_t, shift_left_zero<int8_t> >(data8, SIZE, "int8_t left shift zero");
    
    // unsigned 16 bit
    fill(data16unsigned, data16unsigned+SIZE, uint16_t(init_value));
    test_constant<uint16_t, shift_right_constant<uint16_t> >(data16unsigned, SIZE, "uint16_t constant right shift");
    test_constant<uint16_t, shift_right_repeated_constant<uint16_t> >(data16unsigned, SIZE, "uint16_t repeated constant right shift");
    
    test_constant<uint16_t, shift_left_constant<uint16_t> >(data16unsigned, SIZE, "uint16_t constant left shift");
    test_constant<uint16_t, shift_left_repeated_constant<uint16_t> >(data16unsigned, SIZE, "uint16_t repeated constant left shift");
    
    test_constant<uint16_t, shift_identity<uint16_t> >(data16unsigned, SIZE, "uint16_t identity");
    test_constant<uint16_t, shift_right_zero<uint16_t> >(data16unsigned, SIZE, "uint16_t right shift zero");
    test_constant<uint16_t, shift_left_zero<uint16_t> >(data16unsigned, SIZE, "uint16_t left shift zero");
    
    // signed 16 bit
    fill(data16, data16+SIZE, int16_t(init_value));
    test_constant<int16_t, shift_right_constant<int16_t> >(data16, SIZE, "int16_t constant right shift");
    test_constant<int16_t, shift_right_repeated_constant<int16_t> >(data16, SIZE, "int16_t repeated constant right shift");
    
    test_constant<int16_t, shift_left_constant<int16_t> >(data16, SIZE, "int16_t constant left shift");
    test_constant<int16_t, shift_left_repeated_constant<int16_t> >(data16, SIZE, "int16_t repeated constant left shift");
    
    test_constant<int16_t, shift_identity<int16_t> >(data16, SIZE, "int16_t identity");
    test_constant<int16_t, shift_right_zero<int16_t> >(data16, SIZE, "int16_t right shift zero");
    test_constant<int16_t, shift_left_zero<int16_t> >(data16, SIZE, "int16_t left shift zero");
    
    // unsigned 32 bit
    fill(data32unsigned, data32unsigned+SIZE, uint32_t(init_value));
    test_constant<uint32_t, shift_right_constant<uint32_t> >(data32unsigned, SIZE, "uint32_t constant right shift");
    test_constant<uint32_t, shift_right_repeated_constant<uint32_t> >(data32unsigned, SIZE, "uint32_t repeated constant right shift");
    
    test_constant<uint32_t, shift_left_constant<uint32_t> >(data32unsigned, SIZE, "uint32_t constant left shift");
    test_constant<uint32_t, shift_left_repeated_constant<uint32_t> >(data32unsigned, SIZE, "uint32_t repeated constant left shift");
    
    test_constant<uint32_t, shift_identity<uint32_t> >(data32unsigned, SIZE, "uint32_t identity");
    test_constant<uint32_t, shift_right_zero<uint32_t> >(data32unsigned, SIZE, "uint32_t right shift zero");
    test_constant<uint32_t, shift_left_zero<uint32_t> >(data32unsigned, SIZE, "uint32_t left shift zero");

    // signed 32 bit
    fill(data32, data32+SIZE, int32_t(init_value));
    test_constant<int32_t, shift_right_constant<int32_t> >(data32, SIZE, "int32_t constant right shift");
    test_constant<int32_t, shift_right_repeated_constant<int32_t> >(data32, SIZE, "int32_t repeated constant right shift");
    
    test_constant<int32_t, shift_left_constant<int32_t> >(data32, SIZE, "int32_t constant left shift");
    test_constant<int32_t, shift_left_repeated_constant<int32_t> >(data32, SIZE, "int32_t repeated constant left shift");
    
    test_constant<int32_t, shift_identity<int32_t> >(data32, SIZE, "int32_t identity");
    test_constant<int32_t, shift_right_zero<int32_t> >(data32, SIZE, "int32_t right shift zero");
    test_constant<int32_t, shift_left_zero<int32_t> >(data32, SIZE, "int32_t left shift zero");
    
    // unsigned 64 bit
    fill(data64unsigned, data64unsigned+SIZE, uint64_t(init_value));
    test_constant<uint64_t, shift_right_constant<uint64_t> >(data64unsigned, SIZE, "uint64_t constant right shift");
    test_constant<uint64_t, shift_right_repeated_constant<uint64_t> >(data64unsigned, SIZE, "uint64_t repeated constant right shift");
    
    test_constant<uint64_t, shift_left_constant<uint64_t> >(data64unsigned, SIZE, "uint64_t constant left shift");
    test_constant<uint64_t, shift_left_repeated_constant<uint64_t> >(data64unsigned, SIZE, "uint64_t repeated constant left shift");
    
    test_constant<uint64_t, shift_identity<uint64_t> >(data64unsigned, SIZE, "uint64_t identity");
    test_constant<uint64_t, shift_right_zero<uint64_t> >(data64unsigned, SIZE, "uint64_t right shift zero");
    test_constant<uint64_t, shift_left_zero<uint64_t> >(data64unsigned, SIZE, "uint64_t left shift zero");

    // signed 64 bit
    fill(data64, data64+SIZE, int64_t(init_value));
    test_constant<int64_t, shift_right_constant<int64_t> >(data64, SIZE, "int64_t constant right shift");
    test_constant<int64_t, shift_right_repeated_constant<int64_t> >(data64, SIZE, "int64_t repeated constant right shift");
    
    test_constant<int64_t, shift_left_constant<int64_t> >(data64, SIZE, "int64_t constant left shift");
    test_constant<int64_t, shift_left_repeated_constant<int64_t> >(data64, SIZE, "int64_t repeated constant left shift");
    
    test_constant<int64_t, shift_identity<int64_t> >(data64, SIZE, "int64_t identity");
    test_constant<int64_t, shift_right_zero<int64_t> >(data64, SIZE, "int64_t right shift zero");
    test_constant<int64_t, shift_left_zero<int64_t> >(data64, SIZE, "int64_t left shift zero");
    
      summarize("Multiple Constant Shifts", SIZE, iterations, kDontShowGMeans, kDontShowPenalty );



    // instruction combining multiple variable shifts

    // unsigned 8 bit
    fill(data8unsigned, data8unsigned+SIZE, uint8_t(init_value));
    test_variable1<uint8_t, shift_right_variable<uint8_t> >(data8unsigned, SIZE, 5*shift_factor, "uint8_t variable right shift");
    test_variable1<uint8_t, shift_right_repeated_variable<uint8_t> >(data8unsigned, SIZE, shift_factor, "uint8_t repeated variable right shift");
    
    test_variable1<uint8_t, shift_left_variable<uint8_t> >(data8unsigned, SIZE, 5*shift_factor, "uint8_t variable left shift");
    test_variable1<uint8_t, shift_left_repeated_variable<uint8_t> >(data8unsigned, SIZE, shift_factor, "uint8_t repeated variable left shift");
    
    // signed 8 bit
    fill(data8, data8+SIZE, int8_t(init_value));
    test_variable1<int8_t, shift_right_variable<int8_t> >(data8, SIZE, 5*shift_factor, "int8_t variable right shift");
    test_variable1<int8_t, shift_right_repeated_variable<int8_t> >(data8, SIZE, shift_factor, "int8_t repeated variable right shift");
    
    test_variable1<int8_t, shift_left_variable<int8_t> >(data8, SIZE, 5*shift_factor, "int8_t variable left shift");
    test_variable1<int8_t, shift_left_repeated_variable<int8_t> >(data8, SIZE, shift_factor, "int8_t repeated variable left shift");
    
    // unsigned 16 bit
    fill(data16unsigned, data16unsigned+SIZE, uint16_t(init_value));
    test_variable1<uint16_t, shift_right_variable<uint16_t> >(data16unsigned, SIZE, 5*shift_factor, "uint16_t variable right shift");
    test_variable1<uint16_t, shift_right_repeated_variable<uint16_t> >(data16unsigned, SIZE, shift_factor, "uint16_t repeated variable right shift");
    
    test_variable1<uint16_t, shift_left_variable<uint16_t> >(data16unsigned, SIZE, 5*shift_factor, "uint16_t variable left shift");
    test_variable1<uint16_t, shift_left_repeated_variable<uint16_t> >(data16unsigned, SIZE, shift_factor, "uint16_t repeated variable left shift");
    
    // signed 16 bit
    fill(data16, data16+SIZE, int16_t(init_value));
    test_variable1<int16_t, shift_right_variable<int16_t> >(data16, SIZE, 5*shift_factor, "int16_t variable right shift");
    test_variable1<int16_t, shift_right_repeated_variable<int16_t> >(data16, SIZE, shift_factor, "int16_t repeated variable right shift");
    
    test_variable1<int16_t, shift_left_variable<int16_t> >(data16, SIZE, 5*shift_factor, "int16_t variable left shift");
    test_variable1<int16_t, shift_left_repeated_variable<int16_t> >(data16, SIZE, shift_factor, "int16_t repeated variable left shift");
    
    // unsigned 32 bit
    fill(data32unsigned, data32unsigned+SIZE, uint32_t(init_value));
    test_variable1<uint32_t, shift_right_variable<uint32_t> >(data32unsigned, SIZE, 5*shift_factor, "uint32_t variable right shift");
    test_variable1<uint32_t, shift_right_repeated_variable<uint32_t> >(data32unsigned, SIZE, shift_factor, "uint32_t repeated variable right shift");
    
    test_variable1<uint32_t, shift_left_variable<uint32_t> >(data32unsigned, SIZE, 5*shift_factor, "uint32_t variable left shift");
    test_variable1<uint32_t, shift_left_repeated_variable<uint32_t> >(data32unsigned, SIZE, shift_factor, "uint32_t repeated variable left shift");

    // signed 32 bit
    fill(data32, data32+SIZE, int32_t(init_value));
    test_variable1<int32_t, shift_right_variable<int32_t> >(data32, SIZE, 5*shift_factor, "int32_t variable right shift");
    test_variable1<int32_t, shift_right_repeated_variable<int32_t> >(data32, SIZE, shift_factor, "int32_t repeated variable right shift");
    
    test_variable1<int32_t, shift_left_variable<int32_t> >(data32, SIZE, 5*shift_factor, "int32_t variable left shift");
    test_variable1<int32_t, shift_left_repeated_variable<int32_t> >(data32, SIZE, shift_factor, "int32_t repeated variable left shift");
    
    // unsigned 64 bit
    fill(data64unsigned, data64unsigned+SIZE, uint64_t(init_value));
    test_variable1<uint64_t, shift_right_variable<uint64_t> >(data64unsigned, SIZE, 5*shift_factor, "uint64_t variable right shift");
    test_variable1<uint64_t, shift_right_repeated_variable<uint64_t> >(data64unsigned, SIZE, shift_factor, "uint64_t repeated variable right shift");
    
    test_variable1<uint64_t, shift_left_variable<uint64_t> >(data64unsigned, SIZE, 5*shift_factor, "uint64_t variable left shift");
    test_variable1<uint64_t, shift_left_repeated_variable<uint64_t> >(data64unsigned, SIZE, shift_factor, "uint64_t repeated variable left shift");

    // signed 64 bit
    fill(data64, data64+SIZE, int64_t(init_value));
    test_variable1<int64_t, shift_right_variable<int64_t> >(data64, SIZE, 5*shift_factor, "int64_t variable right shift");
    test_variable1<int64_t, shift_right_repeated_variable<int64_t> >(data64, SIZE, shift_factor, "int64_t repeated variable right shift");
    
    test_variable1<int64_t, shift_left_variable<int64_t> >(data64, SIZE, 5*shift_factor, "int64_t variable left shift");
    test_variable1<int64_t, shift_left_repeated_variable<int64_t> >(data64, SIZE, shift_factor, "int64_t repeated variable left shift");
    
      summarize("Multiple Variable Shifts", SIZE, iterations, kDontShowGMeans, kDontShowPenalty );



    // mask low bits by shift tests
    
    // unsigned 8 bit
    fill(data8unsigned, data8unsigned+SIZE, uint8_t(init_value));
    test_constant<uint8_t, mask_low_constant<uint8_t> >(data8unsigned, SIZE, "uint8_t constant mask low");
    test_constant<uint8_t, mask_low_by_shift_constant<uint8_t> >(data8unsigned, SIZE, "uint8_t constant mask low by shift");

    // signed 8 bit
    fill(data8, data8+SIZE, int8_t(init_value));
    test_constant<int8_t, mask_low_constant<int8_t> >(data8, SIZE, "int8_t constant mask low");
    test_constant<int8_t, mask_low_by_shift_constant<int8_t> >(data8, SIZE, "int8_t constant mask low by shift");
    
    // unsigned 16 bit
    fill(data16unsigned, data16unsigned+SIZE, uint16_t(init_value));
    test_constant<uint16_t, mask_low_constant<uint16_t> >(data16unsigned, SIZE, "uint16_t constant mask low");
    test_constant<uint16_t, mask_low_by_shift_constant<uint16_t> >(data16unsigned, SIZE, "uint16_t constant mask low by shift");

    // signed 16 bit
    fill(data16, data16+SIZE, int16_t(init_value));
    test_constant<int16_t, mask_low_constant<int16_t> >(data16, SIZE, "int16_t constant mask low");
    test_constant<int16_t, mask_low_by_shift_constant<int16_t> >(data16, SIZE, "int16_t constant mask low by shift");
    
    // unsigned 32 bit
    fill(data32unsigned, data32unsigned+SIZE, uint32_t(init_value));
    test_constant<uint32_t, mask_low_constant<uint32_t> >(data32unsigned, SIZE, "uint32_t constant mask low");
    test_constant<uint32_t, mask_low_by_shift_constant<uint32_t> >(data32unsigned, SIZE, "uint32_t constant mask low by shift");

    // signed 32 bit
    fill(data32, data32+SIZE, int32_t(init_value));
    test_constant<int32_t, mask_low_constant<int32_t> >(data32, SIZE, "int32_t constant mask low");
    test_constant<int32_t, mask_low_by_shift_constant<int32_t> >(data32, SIZE, "int32_t constant mask low by shift");
    
    // unsigned 64 bit
    fill(data64unsigned, data64unsigned+SIZE, uint64_t(init_value));
    test_constant<uint64_t, mask_low_constant<uint64_t> >(data64unsigned, SIZE, "uint64_t constant mask low");
    test_constant<uint64_t, mask_low_by_shift_constant<uint64_t> >(data64unsigned, SIZE, "uint64_t constant mask low by shift");

    // signed 64 bit
    fill(data64, data64+SIZE, int64_t(init_value));
    test_constant<int64_t, mask_low_constant<int64_t> >(data64, SIZE, "int64_t constant mask low");
    test_constant<int64_t, mask_low_by_shift_constant<int64_t> >(data64, SIZE, "int64_t constant mask low by shift");

      summarize("Shift Mask Low Constant", SIZE, iterations, kDontShowGMeans, kDontShowPenalty );
    
    
    shift_factor = 4;
    // unsigned 8 bit
    fill(data8unsigned, data8unsigned+SIZE, uint8_t(init_value));
    test_variable1<uint8_t, mask_low_variable<uint8_t> >(data8unsigned, SIZE, shift_factor, "uint8_t variable mask low");
    test_variable1<uint8_t, mask_low_by_shift_variable<uint8_t> >(data8unsigned, SIZE, shift_factor, "uint8_t variable mask low by shift");

    // signed 8 bit
    fill(data8, data8+SIZE, int8_t(init_value));
    test_variable1<int8_t, mask_low_variable<int8_t> >(data8, SIZE, shift_factor, "int8_t variable mask low");
    test_variable1<int8_t, mask_low_by_shift_variable<int8_t> >(data8, SIZE, shift_factor, "int8_t variable mask low by shift");
    
    // unsigned 16 bit
    fill(data16unsigned, data16unsigned+SIZE, uint16_t(init_value));
    test_variable1<uint16_t, mask_low_variable<uint16_t> >(data16unsigned, SIZE, shift_factor, "uint16_t variable mask low");
    test_variable1<uint16_t, mask_low_by_shift_variable<uint16_t> >(data16unsigned, SIZE, shift_factor, "uint16_t variable mask low by shift");

    // signed 16 bit
    fill(data16, data16+SIZE, int16_t(init_value));
    test_variable1<int16_t, mask_low_variable<int16_t> >(data16, SIZE, shift_factor, "int16_t variable mask low");
    test_variable1<int16_t, mask_low_by_shift_variable<int16_t> >(data16, SIZE, shift_factor, "int16_t variable mask low by shift");
    
    // unsigned 32 bit
    fill(data32unsigned, data32unsigned+SIZE, uint32_t(init_value));
    test_variable1<uint32_t, mask_low_variable<uint32_t> >(data32unsigned, SIZE, shift_factor, "uint32_t variable mask low");
    test_variable1<uint32_t, mask_low_by_shift_variable<uint32_t> >(data32unsigned, SIZE, shift_factor, "uint32_t variable mask low by shift");

    // signed 32 bit
    fill(data32, data32+SIZE, int32_t(init_value));
    test_variable1<int32_t, mask_low_variable<int32_t> >(data32, SIZE, shift_factor, "int32_t variable mask low");
    test_variable1<int32_t, mask_low_by_shift_variable<int32_t> >(data32, SIZE, shift_factor, "int32_t variable mask low by shift");
    
    // unsigned 64 bit
    fill(data64unsigned, data64unsigned+SIZE, uint64_t(init_value));
    test_variable1<uint64_t, mask_low_variable<uint64_t> >(data64unsigned, SIZE, shift_factor, "uint64_t variable mask low");
    test_variable1<uint64_t, mask_low_by_shift_variable<uint64_t> >(data64unsigned, SIZE, shift_factor, "uint64_t variable mask low by shift");

    // signed 64 bit
    fill(data64, data64+SIZE, int64_t(init_value));
    test_variable1<int64_t, mask_low_variable<int64_t> >(data64, SIZE, shift_factor, "int64_t variable mask low");
    test_variable1<int64_t, mask_low_by_shift_variable<int64_t> >(data64, SIZE, shift_factor, "int64_t variable mask low by shift");
    
      summarize("Shift Mask Low Variable", SIZE, iterations, kDontShowGMeans, kDontShowPenalty );



    // mask high bits by shift tests
    
    // unsigned 8 bit
    fill(data8unsigned, data8unsigned+SIZE, uint8_t(init_value));
    test_constant<uint8_t, mask_high_constant<uint8_t> >(data8unsigned, SIZE, "uint8_t constant mask high");
    test_constant<uint8_t, mask_high_by_shift_constant<uint8_t> >(data8unsigned, SIZE, "uint8_t constant mask high by shift");
    
    // unsigned 16 bit
    fill(data16unsigned, data16unsigned+SIZE, uint16_t(init_value));
    test_constant<uint16_t, mask_high_constant<uint16_t> >(data16unsigned, SIZE, "uint16_t constant mask high");
    test_constant<uint16_t, mask_high_by_shift_constant<uint16_t> >(data16unsigned, SIZE, "uint16_t constant mask high by shift");
    
    // unsigned 32 bit
    fill(data32unsigned, data32unsigned+SIZE, uint32_t(init_value));
    test_constant<uint32_t, mask_high_constant<uint32_t> >(data32unsigned, SIZE, "uint32_t constant mask high");
    test_constant<uint32_t, mask_high_by_shift_constant<uint32_t> >(data32unsigned, SIZE, "uint32_t constant mask high by shift");
    
    // unsigned 64 bit
    fill(data64unsigned, data64unsigned+SIZE, uint64_t(init_value));
    test_constant<uint64_t, mask_high_constant<uint64_t> >(data64unsigned, SIZE, "uint64_t constant mask high");
    test_constant<uint64_t, mask_high_by_shift_constant<uint64_t> >(data64unsigned, SIZE, "uint64_t constant mask high by shift");

      summarize("Shift Mask High Constant", SIZE, iterations, kDontShowGMeans, kDontShowPenalty );
    
    // unsigned 8 bit
    fill(data8unsigned, data8unsigned+SIZE, uint8_t(init_value));
    test_variable1<uint8_t, mask_high_variable<uint8_t> >(data8unsigned, SIZE, shift_factor, "uint8_t variable mask high");
    test_variable1<uint8_t, mask_high_by_shift_variable<uint8_t> >(data8unsigned, SIZE, shift_factor, "uint8_t variable mask high by shift");
    
    // unsigned 16 bit
    fill(data16unsigned, data16unsigned+SIZE, uint16_t(init_value));
    test_variable1<uint16_t, mask_high_variable<uint16_t> >(data16unsigned, SIZE, shift_factor, "uint16_t variable mask high");
    test_variable1<uint16_t, mask_high_by_shift_variable<uint16_t> >(data16unsigned, SIZE, shift_factor, "uint16_t variable mask high by shift");
    
    // unsigned 32 bit
    fill(data32unsigned, data32unsigned+SIZE, uint32_t(init_value));
    test_variable1<uint32_t, mask_high_variable<uint32_t> >(data32unsigned, SIZE, shift_factor, "uint32_t variable mask high");
    test_variable1<uint32_t, mask_high_by_shift_variable<uint32_t> >(data32unsigned, SIZE, shift_factor, "uint32_t variable mask high by shift");
    
    // unsigned 64 bit
    fill(data64unsigned, data64unsigned+SIZE, uint64_t(init_value));
    test_variable1<uint64_t, mask_high_variable<uint64_t> >(data64unsigned, SIZE, shift_factor, "uint64_t variable mask high");
    test_variable1<uint64_t, mask_high_by_shift_variable<uint64_t> >(data64unsigned, SIZE, shift_factor, "uint64_t variable mask high by shift");

      summarize("Shift Mask High Variable", SIZE, iterations, kDontShowGMeans, kDontShowPenalty );
    
    
    return 0;
}

// the end
/******************************************************************************/
/******************************************************************************/
