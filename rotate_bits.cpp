/*
    Copyright 2007-2008 Adobe Systems Incorporated
    Copyright 2018 Chris Cox
    Distributed under the MIT License (see accompanying file LICENSE_1_0_0.txt
    or a copy at http://stlab.adobe.com/licenses.html )


Goal: Test compiler optimizations related to idioms for bit rotation


Assumptions:

    1) The compiler will recognize common idioms for bit rotation and substitute optimal code.
        On most processors, there are rotate instructions that should be used.


*/

#include "benchmark_stdint.hpp"
#include <cstddef>
#include <cstdio>
#include <ctime>
#include <cstdlib>
#include "benchmark_results.h"
#include "benchmark_timer.h"

/******************************************************************************/

// this constant may need to be adjusted to give reasonable minimum times
// For best results, times should be about 1.0 seconds for the minimum test run
int iterations = 1000000;


// 8000 items, or between 8k and 64k of data
// this is intended to remain within the L2 cache of most common CPUs
const int SIZE = 8000;


// initial value for filling our arrays, may be changed from the command line
uint32_t init_value = 0x55555555;

/******************************************************************************/

// our global arrays of numbers to be operated upon

uint64_t data64unsigned[SIZE];

uint32_t data32unsigned[SIZE];

uint16_t data16unsigned[SIZE];

uint8_t data8unsigned[SIZE];

/******************************************************************************/
/******************************************************************************/

template <typename T>
struct rotate_right_constant {
    static T do_shift(T input) { return ((input >> 5) | (input << ((8*sizeof(T)) - 5))); }
};

/******************************************************************************/

template <typename T>
struct rotate_right_constant2 {
    static T do_shift(T input) { return ((input >> 5) + (input << ((8*sizeof(T)) - 5))); }
};

/******************************************************************************/

// misguided, but I've seen it in real code - and the compiler should be able to simplify
template <typename T>
struct rotate_right_constant3 {
    static T do_shift(T input) { return ( ((input >> 5) & ((~T(0)) >> 5)) | (((input << ((8*sizeof(T)) - 5))) & ~(((~T(0)) >> 5))) ); }
};

/******************************************************************************/

// misguided, but I've seen it in real code - and the compiler should be able to simplify
template <typename T>
struct rotate_right_constant4 {
    static T do_shift(T input) { return ( ((input >> 5) & ((~T(0)) >> 5)) + (((input << ((8*sizeof(T)) - 5))) & ~(((~T(0)) >> 5))) ); }
};

/******************************************************************************/
/******************************************************************************/

template <typename T>
struct rotate_left_constant {
    static T do_shift(T input) { return ((input << 5) | (input >> ((8*sizeof(T)) - 5)) ); }
};

/******************************************************************************/

template <typename T>
struct rotate_left_constant2 {
    static T do_shift(T input) { return ((input << 5) + (input >> ((8*sizeof(T)) - 5)) ); }
};

/******************************************************************************/

// misguided, but I've seen it in real code - and the compiler should be able to simplify
template <typename T>
struct rotate_left_constant3 {
    static T do_shift(T input) { return ( ((input << 5) & ~((~T(0)) >> 5)) | (((input >> ((8*sizeof(T)) - 5))) & (((~T(0)) >> 5))) ); }
};

/******************************************************************************/

// misguided, but I've seen it in real code - and the compiler should be able to simplify
template <typename T>
struct rotate_left_constant4 {
    static T do_shift(T input) { return ( ((input << 5) & ~((~T(0)) >> 5)) + (((input >> ((8*sizeof(T)) - 5))) & (((~T(0)) >> 5))) ); }
};

/******************************************************************************/
/******************************************************************************/

template <typename T>
struct rotate_right_variable {
    static T do_shift(T input, const int shift) { return ((input >> shift) | (input << ((8*sizeof(T)) - shift))); }
};

/******************************************************************************/

template <typename T>
struct rotate_right_variable2 {
    static T do_shift(T input, const int shift) { return ((input >> shift) + (input << ((8*sizeof(T)) - shift))); }
};

/******************************************************************************/

// misguided, but I've seen it in real code - and the compiler should be able to simplify
template <typename T>
struct rotate_right_variable3 {
    static T do_shift(T input, const int shift) { return ( ((input >> shift) & ((~T(0)) >> shift)) | (((input << ((8*sizeof(T)) - shift))) & ~(((~T(0)) >> shift))) ); }
};

/******************************************************************************/

// misguided, but I've seen it in real code - and the compiler should be able to simplify
template <typename T>
struct rotate_right_variable4 {
    static T do_shift(T input, const int shift) { return ( ((input >> shift) & ((~T(0)) >> shift)) + (((input << ((8*sizeof(T)) - shift))) & ~(((~T(0)) >> shift))) ); }
};

/******************************************************************************/
/******************************************************************************/

template <typename T>
struct rotate_left_variable {
    static T do_shift(T input, const int shift) { return ((input << shift) | (input >> ((8*sizeof(T)) - shift))); }
};

/******************************************************************************/

template <typename T>
struct rotate_left_variable2 {
    static T do_shift(T input, const int shift) { return ((input << shift) + (input >> ((8*sizeof(T)) - shift))); }
};

/******************************************************************************/

// misguided, but I've seen it in real code - and the compiler should be able to simplify
template <typename T>
struct rotate_left_variable3 {
    static T do_shift(T input, const int shift) { return ( ((input << shift) & ~((~T(0)) >> shift)) | (((input >> ((8*sizeof(T)) - shift))) & (((~T(0)) >> shift))) ); }
};

/******************************************************************************/

// misguided, but I've seen it in real code - and the compiler should be able to simplify
template <typename T>
struct rotate_left_variable4 {
    static T do_shift(T input, const int shift) { return ( ((input << shift) & ~((~T(0)) >> shift)) + (((input >> ((8*sizeof(T)) - shift))) & (((~T(0)) >> shift))) ); }
};

/******************************************************************************/
/******************************************************************************/

#include "benchmark_shared_tests.h"

/******************************************************************************/

template <typename T, class Shifter>
void test_variable_shift(T* first, int count, const int shift, const char *label) {
  int i;
  
  start_timer();
  
  for(i = 0; i < iterations; ++i) {
    T result = 0;
    for (int n = 0; n < count; ++n) {
        result += Shifter::do_shift( first[n], shift );
    }
    check_shifted_variable_sum<T, Shifter>(result, shift);
  }
  record_result( timer(), label );
}

/******************************************************************************/
/******************************************************************************/

int main(int argc, char** argv) {
    int shift_factor = 5;
  
    // output command for documentation:
    int i;
    for (i = 0; i < argc; ++i)
        printf("%s ", argv[i] );
    printf("\n");

    if (argc > 1) iterations = atoi(argv[1]);
    if (argc > 2) init_value = (long) atoi(argv[2]);
    if (argc > 3) shift_factor = (unsigned int) atoi(argv[3]);

    fill(data8unsigned, data8unsigned+SIZE, uint8_t(init_value));
    fill(data16unsigned, data16unsigned+SIZE, uint16_t(init_value));
    fill(data32unsigned, data32unsigned+SIZE, uint32_t(init_value));
    fill(data64unsigned, data64unsigned+SIZE, uint64_t(init_value));


    test_constant<uint8_t, rotate_right_constant<uint8_t> >(data8unsigned, SIZE, "uint8_t constant right rotate");
    test_constant<uint8_t, rotate_right_constant2<uint8_t> >(data8unsigned, SIZE, "uint8_t constant2 right rotate");
    test_constant<uint8_t, rotate_right_constant3<uint8_t> >(data8unsigned, SIZE, "uint8_t constant3 right rotate");
    test_constant<uint8_t, rotate_right_constant4<uint8_t> >(data8unsigned, SIZE, "uint8_t constant4 right rotate");

    test_constant<uint16_t, rotate_right_constant<uint16_t> >(data16unsigned, SIZE, "uint16_t constant right rotate");
    test_constant<uint16_t, rotate_right_constant2<uint16_t> >(data16unsigned, SIZE, "uint16_t constant2 right rotate");
    test_constant<uint16_t, rotate_right_constant3<uint16_t> >(data16unsigned, SIZE, "uint16_t constant3 right rotate");
    test_constant<uint16_t, rotate_right_constant4<uint16_t> >(data16unsigned, SIZE, "uint16_t constant4 right rotate");
    
    test_constant<uint32_t, rotate_right_constant<uint32_t> >(data32unsigned, SIZE, "uint32_t constant right rotate");
    test_constant<uint32_t, rotate_right_constant2<uint32_t> >(data32unsigned, SIZE, "uint32_t constant2 right rotate");
    test_constant<uint32_t, rotate_right_constant3<uint32_t> >(data32unsigned, SIZE, "uint32_t constant3 right rotate");
    test_constant<uint32_t, rotate_right_constant4<uint32_t> >(data32unsigned, SIZE, "uint32_t constant4 right rotate");
    
    test_constant<uint64_t, rotate_right_constant<uint64_t> >(data64unsigned, SIZE, "uint64_t constant right rotate");
    test_constant<uint64_t, rotate_right_constant2<uint64_t> >(data64unsigned, SIZE, "uint64_t constant2 right rotate");
    test_constant<uint64_t, rotate_right_constant3<uint64_t> >(data64unsigned, SIZE, "uint64_t constant3 right rotate");
    test_constant<uint64_t, rotate_right_constant4<uint64_t> >(data64unsigned, SIZE, "uint64_t constant4 right rotate");
    
      summarize("Constant right rotate", SIZE, iterations, kDontShowGMeans, kDontShowPenalty );


    test_constant<uint8_t, rotate_left_constant<uint8_t> >(data8unsigned, SIZE, "uint8_t constant left rotate");
    test_constant<uint8_t, rotate_left_constant2<uint8_t> >(data8unsigned, SIZE, "uint8_t constant2 left rotate");
    test_constant<uint8_t, rotate_left_constant3<uint8_t> >(data8unsigned, SIZE, "uint8_t constant3 left rotate");
    test_constant<uint8_t, rotate_left_constant4<uint8_t> >(data8unsigned, SIZE, "uint8_t constant4 left rotate");

    test_constant<uint16_t, rotate_left_constant<uint16_t> >(data16unsigned, SIZE, "uint16_t constant left rotate");
    test_constant<uint16_t, rotate_left_constant2<uint16_t> >(data16unsigned, SIZE, "uint16_t constant2 left rotate");
    test_constant<uint16_t, rotate_left_constant3<uint16_t> >(data16unsigned, SIZE, "uint16_t constant3 left rotate");
    test_constant<uint16_t, rotate_left_constant4<uint16_t> >(data16unsigned, SIZE, "uint16_t constant4 left rotate");


    test_constant<uint32_t, rotate_left_constant<uint32_t> >(data32unsigned, SIZE, "uint32_t constant left rotate");
    test_constant<uint32_t, rotate_left_constant2<uint32_t> >(data32unsigned, SIZE, "uint32_t constant2 left rotate");
    test_constant<uint32_t, rotate_left_constant3<uint32_t> >(data32unsigned, SIZE, "uint32_t constant3 left rotate");
    test_constant<uint32_t, rotate_left_constant4<uint32_t> >(data32unsigned, SIZE, "uint32_t constant4 left rotate");

    test_constant<uint64_t, rotate_left_constant<uint64_t> >(data64unsigned, SIZE, "uint64_t constant left rotate");
    test_constant<uint64_t, rotate_left_constant2<uint64_t> >(data64unsigned, SIZE, "uint64_t constant2 left rotate");
    test_constant<uint64_t, rotate_left_constant3<uint64_t> >(data64unsigned, SIZE, "uint64_t constant3 left rotate");
    test_constant<uint64_t, rotate_left_constant4<uint64_t> >(data64unsigned, SIZE, "uint64_t constant4 left rotate");
    
      summarize("Constant left rotate", SIZE, iterations, kDontShowGMeans, kDontShowPenalty );


    test_variable_shift<uint8_t, rotate_right_variable<uint8_t> >(data8unsigned, SIZE, shift_factor, "uint8_t variable right rotate");
    test_variable_shift<uint8_t, rotate_right_variable2<uint8_t> >(data8unsigned, SIZE, shift_factor, "uint8_t variable2 right rotate");
    test_variable_shift<uint8_t, rotate_right_variable3<uint8_t> >(data8unsigned, SIZE, shift_factor, "uint8_t variable3 right rotate");
    test_variable_shift<uint8_t, rotate_right_variable4<uint8_t> >(data8unsigned, SIZE, shift_factor, "uint8_t variable4 right rotate");

    test_variable_shift<uint16_t, rotate_right_variable<uint16_t> >(data16unsigned, SIZE, shift_factor, "uint16_t variable right rotate");
    test_variable_shift<uint16_t, rotate_right_variable2<uint16_t> >(data16unsigned, SIZE, shift_factor, "uint16_t variable2 right rotate");
    test_variable_shift<uint16_t, rotate_right_variable3<uint16_t> >(data16unsigned, SIZE, shift_factor, "uint16_t variable3 right rotate");
    test_variable_shift<uint16_t, rotate_right_variable4<uint16_t> >(data16unsigned, SIZE, shift_factor, "uint16_t variable4 right rotate");
    
    test_variable_shift<uint32_t, rotate_right_variable<uint32_t> >(data32unsigned, SIZE, shift_factor, "uint32_t variable right rotate");
    test_variable_shift<uint32_t, rotate_right_variable2<uint32_t> >(data32unsigned, SIZE, shift_factor, "uint32_t variable2 right rotate");
    test_variable_shift<uint32_t, rotate_right_variable3<uint32_t> >(data32unsigned, SIZE, shift_factor, "uint32_t variable3 right rotate");
    test_variable_shift<uint32_t, rotate_right_variable4<uint32_t> >(data32unsigned, SIZE, shift_factor, "uint32_t variable4 right rotate");
    
    test_variable_shift<uint64_t, rotate_right_variable<uint64_t> >(data64unsigned, SIZE, shift_factor, "uint64_t variable right rotate");
    test_variable_shift<uint64_t, rotate_right_variable2<uint64_t> >(data64unsigned, SIZE, shift_factor, "uint64_t variable2 right rotate");
    test_variable_shift<uint64_t, rotate_right_variable3<uint64_t> >(data64unsigned, SIZE, shift_factor, "uint64_t variable3 right rotate");
    test_variable_shift<uint64_t, rotate_right_variable4<uint64_t> >(data64unsigned, SIZE, shift_factor, "uint64_t variable4 right rotate");

      summarize("Variable right rotate", SIZE, iterations, kDontShowGMeans, kDontShowPenalty );


    test_variable_shift<uint8_t, rotate_left_variable<uint8_t> >(data8unsigned, SIZE, shift_factor, "uint8_t variable left rotate");
    test_variable_shift<uint8_t, rotate_left_variable2<uint8_t> >(data8unsigned, SIZE, shift_factor, "uint8_t variable2 left rotate");
    test_variable_shift<uint8_t, rotate_left_variable3<uint8_t> >(data8unsigned, SIZE, shift_factor, "uint8_t variable3 left rotate");
    test_variable_shift<uint8_t, rotate_left_variable4<uint8_t> >(data8unsigned, SIZE, shift_factor, "uint8_t variable4 left rotate");

    test_variable_shift<uint16_t, rotate_left_variable<uint16_t> >(data16unsigned, SIZE, shift_factor, "uint16_t variable left rotate");
    test_variable_shift<uint16_t, rotate_left_variable2<uint16_t> >(data16unsigned, SIZE, shift_factor, "uint16_t variable2 left rotate");
    test_variable_shift<uint16_t, rotate_left_variable3<uint16_t> >(data16unsigned, SIZE, shift_factor, "uint16_t variable3 left rotate");
    test_variable_shift<uint16_t, rotate_left_variable4<uint16_t> >(data16unsigned, SIZE, shift_factor, "uint16_t variable4 left rotate");
    
    test_variable_shift<uint32_t, rotate_left_variable<uint32_t> >(data32unsigned, SIZE, shift_factor, "uint32_t variable left rotate");
    test_variable_shift<uint32_t, rotate_left_variable2<uint32_t> >(data32unsigned, SIZE, shift_factor, "uint32_t variable2 left rotate");
    test_variable_shift<uint32_t, rotate_left_variable3<uint32_t> >(data32unsigned, SIZE, shift_factor, "uint32_t variable3 left rotate");
    test_variable_shift<uint32_t, rotate_left_variable4<uint32_t> >(data32unsigned, SIZE, shift_factor, "uint32_t variable4 left rotate");
    
    test_variable_shift<uint64_t, rotate_left_variable<uint64_t> >(data64unsigned, SIZE, shift_factor, "uint64_t variable left rotate");
    test_variable_shift<uint64_t, rotate_left_variable2<uint64_t> >(data64unsigned, SIZE, shift_factor, "uint64_t variable2 left rotate");
    test_variable_shift<uint64_t, rotate_left_variable3<uint64_t> >(data64unsigned, SIZE, shift_factor, "uint64_t variable3 left rotate");
    test_variable_shift<uint64_t, rotate_left_variable4<uint64_t> >(data64unsigned, SIZE, shift_factor, "uint64_t variable4 left rotate");

      summarize("Variable left rotate", SIZE, iterations, kDontShowGMeans, kDontShowPenalty );
    
    
    
    return 0;
}

// the end
/******************************************************************************/
/******************************************************************************/
