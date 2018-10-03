/*
    Copyright 2007-2008 Adobe Systems Incorporated
    Copyright 2018 Chris Cox
    Distributed under the MIT License (see accompanying file LICENSE_1_0_0.txt
    or a copy at http://stlab.adobe.com/licenses.html )


Goal: Test compiler optimizations related to scalar replacement of array references.

Assumptions:

    1) The compiler will convert array references to scalar calculations when necessary.
        input[0] += 2;        ==>        input[0] += 14;
        input[0] += 5;
        input[0] += 7;

    2) The compiler will do conversion (1) on local arrays, array arguments, and external arrays.
    
    3) The compiler will apply further optimization to the resulting values.
        Best case: the loops disappear.



NOTE - Someone complained that they did not have enough registers to optimize this with 11 array values.
        But their compiler also fails at 5 array values (and 16 registers).

*/

/******************************************************************************/

#include <stddef.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <math.h>
#include <deque>
#include <string>
#include "benchmark_results.h"
#include "benchmark_timer.h"
#include "benchmark_stdint.hpp"
#include "benchmark_typenames.h"

/******************************************************************************/

// this constant may need to be adjusted to give reasonable minimum times
// For best results, times should be about 1.0 seconds for the minimum test run
int iterations = 900000000;


const int SIZE  = 11;


// initial value for filling our arrays, may be changed from the command line
int32_t init_value = 1;

/******************************************************************************/

std::deque<std::string> gLabels;

/******************************************************************************/

// our global arrays of numbers to be operated upon

int16_t data16[SIZE];
int32_t data32[SIZE];
uint64_t data64[SIZE];
double dataDouble[SIZE];

/******************************************************************************/

template <class Iterator, class T>
void fill(Iterator first, Iterator last, T value) {
  while (first != last) *first++ = value;
}

/******************************************************************************/

template<class T>
inline void check11_sums( T *input, const std::string &label  ) {

    T temp0 = input[0];
    T temp1 = input[1];
    T temp2 = input[2];
    T temp3 = input[3];
    T temp4 = input[4];
    T temp5 = input[5];
    T temp6 = input[6];
    T temp7 = input[7];
    T temp8 = input[8];
    T temp9 = input[9];
    T temp10 = input[10];
    
    T temp0_base = (T)init_value + (T)iterations * 52;
    T temp1_base = (T)init_value + (T)iterations * 40;
    T temp2_base = (T)init_value + (T)iterations * 36;
    T temp3_base = (T)init_value + (T)iterations * 20;
    T temp4_base = (T)init_value + (T)iterations * 36;
    T temp5_base = (T)init_value + (T)iterations * 9;
    T temp6_base = (T)init_value + (T)iterations * 15;
    T temp7_base = (T)init_value + (T)iterations * 30;
    T temp8_base = (T)init_value + (T)iterations * 57;
    T temp9_base = (T)init_value + (T)iterations * 55;
    T temp10_base = (T)init_value + (T)iterations * 60;

    if (temp0 != temp0_base
        || temp1 != temp1_base
        || temp2 != temp2_base
        || temp3 != temp3_base
        || temp4 != temp4_base
        || temp5 != temp5_base
        || temp6 != temp6_base
        || temp7 != temp7_base
        || temp8 != temp8_base
        || temp9 != temp9_base
        || temp10 != temp10_base)
        printf("test %s failed\n", label.c_str() );
}

/******************************************************************************/

// fully optimized - loops disappear
template <typename T>
void test_array11_arg0(T input[SIZE], const std::string &label) {
  
    start_timer();
    
    T temp0 = input[0];
    T temp1 = input[1];
    T temp2 = input[2];
    T temp3 = input[3];
    T temp4 = input[4];
    T temp5 = input[5];
    T temp6 = input[6];
    T temp7 = input[7];
    T temp8 = input[8];
    T temp9 = input[9];
    T temp10 = input[10];
    
    temp0 += (T)52 * (T)iterations;
    temp1 += (T)40 * (T)iterations;
    temp2 += (T)36 * (T)iterations;
    temp3 += (T)20 * (T)iterations;
    temp4 += (T)36 * (T)iterations;
    temp5 += (T)9 * (T)iterations;
    temp6 += (T)15 * (T)iterations;
    temp7 += (T)30 * (T)iterations;
    temp8 += (T)57 * (T)iterations;
    temp9 += (T)55 * (T)iterations;
    temp10 += (T)60 * (T)iterations;
    
    input[0] = temp0;
    input[1] = temp1;
    input[2] = temp2;
    input[3] = temp3;
    input[4] = temp4;
    input[5] = temp5;
    input[6] = temp6;
    input[7] = temp7;
    input[8] = temp8;
    input[9] = temp9;
    input[10] = temp10;
    check11_sums(input, label);
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

// mostly optimized - scalar replacement, collapse terms, but still has a loop
template <typename T>
void test_array11_arg1(T input[SIZE], const std::string &label) {
    int i;
    
    start_timer();
    
    T temp0 = input[0];
    T temp1 = input[1];
    T temp2 = input[2];
    T temp3 = input[3];
    T temp4 = input[4];
    T temp5 = input[5];
    T temp6 = input[6];
    T temp7 = input[7];
    T temp8 = input[8];
    T temp9 = input[9];
    T temp10 = input[10];
  
    for(i = 0; i < iterations; ++i) {
        temp0 += 52;
        temp1 += 40;
        temp2 += 36;
        temp3 += 20;
        temp4 += 36;
        temp5 += 9;
        temp6 += 15;
        temp7 += 30;
        temp8 += 57;
        temp9 += 55;
        temp10 += 60;
    }
    
    input[0] = temp0;
    input[1] = temp1;
    input[2] = temp2;
    input[3] = temp3;
    input[4] = temp4;
    input[5] = temp5;
    input[6] = temp6;
    input[7] = temp7;
    input[8] = temp8;
    input[9] = temp9;
    input[10] = temp10;
    check11_sums(input, label);
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

// barely optimized - basic scalar replacement, terms uncollapsed, still has a loop
template <typename T>
void test_array11_arg2(T input[SIZE], const std::string &label) {
    int i;
    
    start_timer();
    
    T temp0 = input[0];
    T temp1 = input[1];
    T temp2 = input[2];
    T temp3 = input[3];
    T temp4 = input[4];
    T temp5 = input[5];
    T temp6 = input[6];
    T temp7 = input[7];
    T temp8 = input[8];
    T temp9 = input[9];
    T temp10 = input[10];
  
    for(i = 0; i < iterations; ++i) {
        temp8 += 12;
        temp0 += 2;
        temp8 += 6;
        temp6 += 4;
        temp10 += 3;
        temp2 += 1;
        temp4 += 1;
        temp1 += 4;
        temp3 += 2;
        temp7 += 5;
        temp8 += 5;
        temp6 += 3;
        temp0 += 5;
        temp9 += 7;
        temp2 += 3;
        temp5 += 3;
        temp4 += 2;
        temp3 += 4;
        temp6 += 2;
        temp10 += 6;
        temp0 += 7;
        temp1 += 8;
        temp4 += 3;
        temp9 += 9;
        temp2 += 5;
        temp10 += 9;
        temp8 += 11;
        temp4 += 4;
        temp6 += 1;
        temp5 += 3;
        temp0 += 10;
        temp10 += 11;
        temp8 += 9;
        temp1 += 12;
        temp9 += 11;
        temp3 += 6;
        temp4 += 5;
        temp6 += 2;
        temp2 += 7;
        temp0 += 13;
        temp9 += 13;
        temp4 += 6;
        temp10 += 14;
        temp2 += 9;
        temp8 += 4;
        temp5 += 3;
        temp0 += 15;
        temp3 += 8;
        temp7 += 10;
        temp6 += 3;
        temp4 += 7;
        temp8 += 10;
        temp2 += 11;
        temp1 += 16;
        temp4 += 8;
        temp10 += 17;
        temp7 += 15;
        temp9 += 15;
    }
    
    input[0] = temp0;
    input[1] = temp1;
    input[2] = temp2;
    input[3] = temp3;
    input[4] = temp4;
    input[5] = temp5;
    input[6] = temp6;
    input[7] = temp7;
    input[8] = temp8;
    input[9] = temp9;
    input[10] = temp10;
    
    check11_sums(input, label);
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

// unoptimized
template <typename T>
void test_array11_arg(T input[SIZE], const std::string &label) {
    int i;
  
    start_timer();
  
    for(i = 0; i < iterations; ++i) {
        
        input[8] += 12;
        input[0] += 2;
        input[8] += 6;
        input[6] += 4;
        input[10] += 3;
        input[2] += 1;
        input[4] += 1;
        input[1] += 4;
        input[3] += 2;
        input[7] += 5;
        input[8] += 5;
        input[6] += 3;
        input[0] += 5;
        input[9] += 7;
        input[2] += 3;
        input[5] += 3;
        input[4] += 2;
        input[3] += 4;
        input[6] += 2;
        input[10] += 6;
        input[0] += 7;
        input[1] += 8;
        input[4] += 3;
        input[9] += 9;
        input[2] += 5;
        input[10] += 9;
        input[8] += 11;
        input[4] += 4;
        input[6] += 1;
        input[5] += 3;
        input[0] += 10;
        input[10] += 11;
        input[8] += 9;
        input[1] += 12;
        input[9] += 11;
        input[3] += 6;
        input[4] += 5;
        input[6] += 2;
        input[2] += 7;
        input[0] += 13;
        input[9] += 13;
        input[4] += 6;
        input[10] += 14;
        input[2] += 9;
        input[8] += 4;
        input[5] += 3;
        input[0] += 15;
        input[3] += 8;
        input[7] += 10;
        input[6] += 3;
        input[4] += 7;
        input[8] += 10;
        input[2] += 11;
        input[1] += 16;
        input[4] += 8;
        input[10] += 17;
        input[7] += 15;
        input[9] += 15;
        
    }
    
    check11_sums(input, label);
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

// fully optimized - loops disappear
template <typename T>
void test_array11_local0(const std::string &label) {
    
    T input[SIZE];
    ::fill(input, input+SIZE, T(init_value));
  
    start_timer();
    
    T temp0 = input[0];
    T temp1 = input[1];
    T temp2 = input[2];
    T temp3 = input[3];
    T temp4 = input[4];
    T temp5 = input[5];
    T temp6 = input[6];
    T temp7 = input[7];
    T temp8 = input[8];
    T temp9 = input[9];
    T temp10 = input[10];
    
    temp0 += (T)52 * (T)iterations;
    temp1 += (T)40 * (T)iterations;
    temp2 += (T)36 * (T)iterations;
    temp3 += (T)20 * (T)iterations;
    temp4 += (T)36 * (T)iterations;
    temp5 += (T)9 * (T)iterations;
    temp6 += (T)15 * (T)iterations;
    temp7 += (T)30 * (T)iterations;
    temp8 += (T)57 * (T)iterations;
    temp9 += (T)55 * (T)iterations;
    temp10 += (T)60 * (T)iterations;

    input[0] = temp0;
    input[1] = temp1;
    input[2] = temp2;
    input[3] = temp3;
    input[4] = temp4;
    input[5] = temp5;
    input[6] = temp6;
    input[7] = temp7;
    input[8] = temp8;
    input[9] = temp9;
    input[10] = temp10;

    check11_sums(input, label);
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

// mostly optimized - scalar replacement, collapse terms, but still has a loop
template <typename T>
void test_array11_local1(const std::string &label) {
    int i;
    
    T input[SIZE];
    ::fill(input, input+SIZE, T(init_value));
  
    start_timer();
    
    T temp0 = input[0];
    T temp1 = input[1];
    T temp2 = input[2];
    T temp3 = input[3];
    T temp4 = input[4];
    T temp5 = input[5];
    T temp6 = input[6];
    T temp7 = input[7];
    T temp8 = input[8];
    T temp9 = input[9];
    T temp10 = input[10];
  
    for(i = 0; i < iterations; ++i) {
        temp0 += 52;
        temp1 += 40;
        temp2 += 36;
        temp3 += 20;
        temp4 += 36;
        temp5 += 9;
        temp6 += 15;
        temp7 += 30;
        temp8 += 57;
        temp9 += 55;
        temp10 += 60;
    }

    input[0] = temp0;
    input[1] = temp1;
    input[2] = temp2;
    input[3] = temp3;
    input[4] = temp4;
    input[5] = temp5;
    input[6] = temp6;
    input[7] = temp7;
    input[8] = temp8;
    input[9] = temp9;
    input[10] = temp10;

    check11_sums(input, label);
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

// barely optimized - scalar replacement, terms uncollapsed, still has a loop
template <typename T>
void test_array11_local2(const std::string &label) {
    int i;
    
    T input[SIZE];
    ::fill(input, input+SIZE, T(init_value));
  
    start_timer();
    
    T temp0 = input[0];
    T temp1 = input[1];
    T temp2 = input[2];
    T temp3 = input[3];
    T temp4 = input[4];
    T temp5 = input[5];
    T temp6 = input[6];
    T temp7 = input[7];
    T temp8 = input[8];
    T temp9 = input[9];
    T temp10 = input[10];
  
    for(i = 0; i < iterations; ++i) {
        temp8 += 12;
        temp0 += 2;
        temp8 += 6;
        temp6 += 4;
        temp10 += 3;
        temp2 += 1;
        temp4 += 1;
        temp1 += 4;
        temp3 += 2;
        temp7 += 5;
        temp8 += 5;
        temp6 += 3;
        temp0 += 5;
        temp9 += 7;
        temp2 += 3;
        temp5 += 3;
        temp4 += 2;
        temp3 += 4;
        temp6 += 2;
        temp10 += 6;
        temp0 += 7;
        temp1 += 8;
        temp4 += 3;
        temp9 += 9;
        temp2 += 5;
        temp10 += 9;
        temp8 += 11;
        temp4 += 4;
        temp6 += 1;
        temp5 += 3;
        temp0 += 10;
        temp10 += 11;
        temp8 += 9;
        temp1 += 12;
        temp9 += 11;
        temp3 += 6;
        temp4 += 5;
        temp6 += 2;
        temp2 += 7;
        temp0 += 13;
        temp9 += 13;
        temp4 += 6;
        temp10 += 14;
        temp2 += 9;
        temp8 += 4;
        temp5 += 3;
        temp0 += 15;
        temp3 += 8;
        temp7 += 10;
        temp6 += 3;
        temp4 += 7;
        temp8 += 10;
        temp2 += 11;
        temp1 += 16;
        temp4 += 8;
        temp10 += 17;
        temp7 += 15;
        temp9 += 15;
    }

    input[0] = temp0;
    input[1] = temp1;
    input[2] = temp2;
    input[3] = temp3;
    input[4] = temp4;
    input[5] = temp5;
    input[6] = temp6;
    input[7] = temp7;
    input[8] = temp8;
    input[9] = temp9;
    input[10] = temp10;

    check11_sums(input, label);
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

template <typename T>
void test_array11_local(const std::string &label) {
    int i;
    
    T input[SIZE];
    ::fill(input, input+SIZE, T(init_value));
  
    start_timer();
  
    for(i = 0; i < iterations; ++i) {
        
        input[8] += 12;
        input[0] += 2;
        input[8] += 6;
        input[6] += 4;
        input[10] += 3;
        input[2] += 1;
        input[4] += 1;
        input[1] += 4;
        input[3] += 2;
        input[7] += 5;
        input[8] += 5;
        input[6] += 3;
        input[0] += 5;
        input[9] += 7;
        input[2] += 3;
        input[5] += 3;
        input[4] += 2;
        input[3] += 4;
        input[6] += 2;
        input[10] += 6;
        input[0] += 7;
        input[1] += 8;
        input[4] += 3;
        input[9] += 9;
        input[2] += 5;
        input[10] += 9;
        input[8] += 11;
        input[4] += 4;
        input[6] += 1;
        input[5] += 3;
        input[0] += 10;
        input[10] += 11;
        input[8] += 9;
        input[1] += 12;
        input[9] += 11;
        input[3] += 6;
        input[4] += 5;
        input[6] += 2;
        input[2] += 7;
        input[0] += 13;
        input[9] += 13;
        input[4] += 6;
        input[10] += 14;
        input[2] += 9;
        input[8] += 4;
        input[5] += 3;
        input[0] += 15;
        input[3] += 8;
        input[7] += 10;
        input[6] += 3;
        input[4] += 7;
        input[8] += 10;
        input[2] += 11;
        input[1] += 16;
        input[4] += 8;
        input[10] += 17;
        input[7] += 15;
        input[9] += 15;
        
    }
    
    check11_sums(input, label);
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

// fully optimized - loops disappear
template <typename T, T *gInput>
void test_array11_global0(const std::string &label) {
  
    start_timer();
  
    T temp0 = gInput[0];
    T temp1 = gInput[1];
    T temp2 = gInput[2];
    T temp3 = gInput[3];
    T temp4 = gInput[4];
    T temp5 = gInput[5];
    T temp6 = gInput[6];
    T temp7 = gInput[7];
    T temp8 = gInput[8];
    T temp9 = gInput[9];
    T temp10 = gInput[10];
    
    temp0 += (T)52 * (T)iterations;
    temp1 += (T)40 * (T)iterations;
    temp2 += (T)36 * (T)iterations;
    temp3 += (T)20 * (T)iterations;
    temp4 += (T)36 * (T)iterations;
    temp5 += (T)9 * (T)iterations;
    temp6 += (T)15 * (T)iterations;
    temp7 += (T)30 * (T)iterations;
    temp8 += (T)57 * (T)iterations;
    temp9 += (T)55 * (T)iterations;
    temp10 += (T)60 * (T)iterations;

    gInput[0] = temp0;
    gInput[1] = temp1;
    gInput[2] = temp2;
    gInput[3] = temp3;
    gInput[4] = temp4;
    gInput[5] = temp5;
    gInput[6] = temp6;
    gInput[7] = temp7;
    gInput[8] = temp8;
    gInput[9] = temp9;
    gInput[10] = temp10;

    check11_sums(gInput, label);
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

// mostly optimized - scalar replacement, collapse terms, but still has a loop
template <typename T, T *gInput>
void test_array11_global1(const std::string &label) {
    int i;
    
    start_timer();
  
    T temp0 = gInput[0];
    T temp1 = gInput[1];
    T temp2 = gInput[2];
    T temp3 = gInput[3];
    T temp4 = gInput[4];
    T temp5 = gInput[5];
    T temp6 = gInput[6];
    T temp7 = gInput[7];
    T temp8 = gInput[8];
    T temp9 = gInput[9];
    T temp10 = gInput[10];
    
    for(i = 0; i < iterations; ++i) {
        temp0 += 52;
        temp1 += 40;
        temp2 += 36;
        temp3 += 20;
        temp4 += 36;
        temp5 += 9;
        temp6 += 15;
        temp7 += 30;
        temp8 += 57;
        temp9 += 55;
        temp10 += 60;
    }

    gInput[0] = temp0;
    gInput[1] = temp1;
    gInput[2] = temp2;
    gInput[3] = temp3;
    gInput[4] = temp4;
    gInput[5] = temp5;
    gInput[6] = temp6;
    gInput[7] = temp7;
    gInput[8] = temp8;
    gInput[9] = temp9;
    gInput[10] = temp10;

    check11_sums(gInput, label);
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

// barely optimized - scalar replacement, terms uncollapsed, still has a loop
template <typename T, T *gInput>
void test_array11_global2(const std::string &label) {
    int i;
    
    start_timer();
  
    T temp0 = gInput[0];
    T temp1 = gInput[1];
    T temp2 = gInput[2];
    T temp3 = gInput[3];
    T temp4 = gInput[4];
    T temp5 = gInput[5];
    T temp6 = gInput[6];
    T temp7 = gInput[7];
    T temp8 = gInput[8];
    T temp9 = gInput[9];
    T temp10 = gInput[10];
    
    for(i = 0; i < iterations; ++i) {
        temp8 += 12;
        temp0 += 2;
        temp8 += 6;
        temp6 += 4;
        temp10 += 3;
        temp2 += 1;
        temp4 += 1;
        temp1 += 4;
        temp3 += 2;
        temp7 += 5;
        temp8 += 5;
        temp6 += 3;
        temp0 += 5;
        temp9 += 7;
        temp2 += 3;
        temp5 += 3;
        temp4 += 2;
        temp3 += 4;
        temp6 += 2;
        temp10 += 6;
        temp0 += 7;
        temp1 += 8;
        temp4 += 3;
        temp9 += 9;
        temp2 += 5;
        temp10 += 9;
        temp8 += 11;
        temp4 += 4;
        temp6 += 1;
        temp5 += 3;
        temp0 += 10;
        temp10 += 11;
        temp8 += 9;
        temp1 += 12;
        temp9 += 11;
        temp3 += 6;
        temp4 += 5;
        temp6 += 2;
        temp2 += 7;
        temp0 += 13;
        temp9 += 13;
        temp4 += 6;
        temp10 += 14;
        temp2 += 9;
        temp8 += 4;
        temp5 += 3;
        temp0 += 15;
        temp3 += 8;
        temp7 += 10;
        temp6 += 3;
        temp4 += 7;
        temp8 += 10;
        temp2 += 11;
        temp1 += 16;
        temp4 += 8;
        temp10 += 17;
        temp7 += 15;
        temp9 += 15;
    }

    gInput[0] = temp0;
    gInput[1] = temp1;
    gInput[2] = temp2;
    gInput[3] = temp3;
    gInput[4] = temp4;
    gInput[5] = temp5;
    gInput[6] = temp6;
    gInput[7] = temp7;
    gInput[8] = temp8;
    gInput[9] = temp9;
    gInput[10] = temp10;

    check11_sums(gInput, label);
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

template <typename T, T *gInput>
void test_array11_global(const std::string &label) {
    int i;
  
    start_timer();
  
    for(i = 0; i < iterations; ++i) {
        
        gInput[8] += 12;
        gInput[0] += 2;
        gInput[8] += 6;
        gInput[6] += 4;
        gInput[10] += 3;
        gInput[2] += 1;
        gInput[4] += 1;
        gInput[1] += 4;
        gInput[3] += 2;
        gInput[7] += 5;
        gInput[8] += 5;
        gInput[6] += 3;
        gInput[0] += 5;
        gInput[9] += 7;
        gInput[2] += 3;
        gInput[5] += 3;
        gInput[4] += 2;
        gInput[3] += 4;
        gInput[6] += 2;
        gInput[10] += 6;
        gInput[0] += 7;
        gInput[1] += 8;
        gInput[4] += 3;
        gInput[9] += 9;
        gInput[2] += 5;
        gInput[10] += 9;
        gInput[8] += 11;
        gInput[4] += 4;
        gInput[6] += 1;
        gInput[5] += 3;
        gInput[0] += 10;
        gInput[10] += 11;
        gInput[8] += 9;
        gInput[1] += 12;
        gInput[9] += 11;
        gInput[3] += 6;
        gInput[4] += 5;
        gInput[6] += 2;
        gInput[2] += 7;
        gInput[0] += 13;
        gInput[9] += 13;
        gInput[4] += 6;
        gInput[10] += 14;
        gInput[2] += 9;
        gInput[8] += 4;
        gInput[5] += 3;
        gInput[0] += 15;
        gInput[3] += 8;
        gInput[7] += 10;
        gInput[6] += 3;
        gInput[4] += 7;
        gInput[8] += 10;
        gInput[2] += 11;
        gInput[1] += 16;
        gInput[4] += 8;
        gInput[10] += 17;
        gInput[7] += 15;
        gInput[9] += 15;
        
    }
    
    check11_sums(gInput, label);
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

template<class T>
inline void check5_sums( T *input, const std::string &label ) {

    T temp0 = input[0];
    T temp1 = input[1];
    T temp2 = input[2];
    T temp3 = input[3];
    T temp4 = input[4];
    
    T temp0_base = (T)init_value + (T)iterations * 52;
    T temp1_base = (T)init_value + (T)iterations * 40;
    T temp2_base = (T)init_value + (T)iterations * 36;
    T temp3_base = (T)init_value + (T)iterations * 20;
    T temp4_base = (T)init_value + (T)iterations * 36;

    if (temp0 != temp0_base
        || temp1 != temp1_base
        || temp2 != temp2_base
        || temp3 != temp3_base
        || temp4 != temp4_base)
        printf("test %s failed\n", label.c_str() );
}

/******************************************************************************/

// fully optimized - loops disappear
template <typename T>
void test_array5_arg0(T input[SIZE], const std::string &label) {
  
    start_timer();
    
    T temp0 = input[0];
    T temp1 = input[1];
    T temp2 = input[2];
    T temp3 = input[3];
    T temp4 = input[4];
    
    temp0 += (T)52 * (T)iterations;
    temp1 += (T)40 * (T)iterations;
    temp2 += (T)36 * (T)iterations;
    temp3 += (T)20 * (T)iterations;
    temp4 += (T)36 * (T)iterations;
    
    input[0] = temp0;
    input[1] = temp1;
    input[2] = temp2;
    input[3] = temp3;
    input[4] = temp4;
    
    check5_sums(input, label);
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

// mostly optimized - scalar replacement, collapse terms, but still has a loop
template <typename T>
void test_array5_arg1(T input[SIZE], const std::string &label) {
    int i;
    
    start_timer();
    
    T temp0 = input[0];
    T temp1 = input[1];
    T temp2 = input[2];
    T temp3 = input[3];
    T temp4 = input[4];
  
    for(i = 0; i < iterations; ++i) {
        temp0 += 52;
        temp1 += 40;
        temp2 += 36;
        temp3 += 20;
        temp4 += 36;
    }
    
    input[0] = temp0;
    input[1] = temp1;
    input[2] = temp2;
    input[3] = temp3;
    input[4] = temp4;
    
    check5_sums(input, label);
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

// barely optimized - basic scalar replacement, terms uncollapsed, still has a loop
template <typename T>
void test_array5_arg2(T input[SIZE], const std::string &label) {
    int i;
    
    start_timer();
    
    T temp0 = input[0];
    T temp1 = input[1];
    T temp2 = input[2];
    T temp3 = input[3];
    T temp4 = input[4];
  
    for(i = 0; i < iterations; ++i) {
        temp0 += 2;
        temp2 += 1;
        temp4 += 1;
        temp1 += 4;
        temp3 += 2;
        temp0 += 5;
        temp2 += 3;
        temp4 += 2;
        temp3 += 4;
        temp0 += 7;
        temp1 += 8;
        temp4 += 3;
        temp2 += 5;
        temp4 += 4;
        temp0 += 10;
        temp1 += 12;
        temp3 += 6;
        temp4 += 5;
        temp2 += 7;
        temp0 += 13;
        temp4 += 6;
        temp2 += 9;
        temp0 += 15;
        temp3 += 8;
        temp4 += 7;
        temp2 += 11;
        temp1 += 16;
        temp4 += 8;
    }
    
    input[0] = temp0;
    input[1] = temp1;
    input[2] = temp2;
    input[3] = temp3;
    input[4] = temp4;
    
    check5_sums(input, label);
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

// unoptimized
template <typename T>
void test_array5_arg(T input[SIZE], const std::string &label) {
    int i;
  
    start_timer();
  
    for(i = 0; i < iterations; ++i) {
        
        input[0] += 2;
        input[2] += 1;
        input[4] += 1;
        input[1] += 4;
        input[3] += 2;
        input[0] += 5;
        input[2] += 3;
        input[4] += 2;
        input[3] += 4;
        input[0] += 7;
        input[1] += 8;
        input[4] += 3;
        input[2] += 5;
        input[4] += 4;
        input[0] += 10;
        input[1] += 12;
        input[3] += 6;
        input[4] += 5;
        input[2] += 7;
        input[0] += 13;
        input[4] += 6;
        input[2] += 9;
        input[0] += 15;
        input[3] += 8;
        input[4] += 7;
        input[2] += 11;
        input[1] += 16;
        input[4] += 8;
        
    }
    
    check5_sums(input, label);
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

// fully optimized - loops disappear
template <typename T>
void test_array5_local0(const std::string &label) {
    
    T input[SIZE];
    ::fill(input, input+SIZE, T(init_value));
  
    start_timer();
    
    T temp0 = input[0];
    T temp1 = input[1];
    T temp2 = input[2];
    T temp3 = input[3];
    T temp4 = input[4];
    
    temp0 += (T)52 * (T)iterations;
    temp1 += (T)40 * (T)iterations;
    temp2 += (T)36 * (T)iterations;
    temp3 += (T)20 * (T)iterations;
    temp4 += (T)36 * (T)iterations;

    input[0] = temp0;
    input[1] = temp1;
    input[2] = temp2;
    input[3] = temp3;
    input[4] = temp4;
    
    check5_sums(input, label);
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

// mostly optimized - scalar replacement, collapse terms, but still has a loop
template <typename T>
void test_array5_local1(const std::string &label) {
    int i;
    
    T input[SIZE];
    ::fill(input, input+SIZE, T(init_value));
  
    start_timer();
    
    T temp0 = input[0];
    T temp1 = input[1];
    T temp2 = input[2];
    T temp3 = input[3];
    T temp4 = input[4];
  
    for(i = 0; i < iterations; ++i) {
        temp0 += 52;
        temp1 += 40;
        temp2 += 36;
        temp3 += 20;
        temp4 += 36;
    }

    input[0] = temp0;
    input[1] = temp1;
    input[2] = temp2;
    input[3] = temp3;
    input[4] = temp4;
    
    check5_sums(input, label);
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

// barely optimized - scalar replacement, terms uncollapsed, still has a loop
template <typename T>
void test_array5_local2(const std::string &label) {
    int i;
    
    T input[SIZE];
    ::fill(input, input+SIZE, T(init_value));
  
    start_timer();
    
    T temp0 = input[0];
    T temp1 = input[1];
    T temp2 = input[2];
    T temp3 = input[3];
    T temp4 = input[4];
  
    for(i = 0; i < iterations; ++i) {
        temp0 += 2;
        temp2 += 1;
        temp4 += 1;
        temp1 += 4;
        temp3 += 2;
        temp0 += 5;
        temp2 += 3;
        temp4 += 2;
        temp3 += 4;
        temp0 += 7;
        temp1 += 8;
        temp4 += 3;
        temp2 += 5;
        temp4 += 4;
        temp0 += 10;
        temp1 += 12;
        temp3 += 6;
        temp4 += 5;
        temp2 += 7;
        temp0 += 13;
        temp4 += 6;
        temp2 += 9;
        temp0 += 15;
        temp3 += 8;
        temp4 += 7;
        temp2 += 11;
        temp1 += 16;
        temp4 += 8;
    }

    input[0] = temp0;
    input[1] = temp1;
    input[2] = temp2;
    input[3] = temp3;
    input[4] = temp4;
    
    check5_sums(input, label);
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

template <typename T>
void test_array5_local(const std::string &label) {
    int i;
    
    T input[SIZE];
    ::fill(input, input+SIZE, T(init_value));
  
    start_timer();
  
    for(i = 0; i < iterations; ++i) {
        
        input[0] += 2;
        input[2] += 1;
        input[4] += 1;
        input[1] += 4;
        input[3] += 2;
        input[0] += 5;
        input[2] += 3;
        input[4] += 2;
        input[3] += 4;
        input[0] += 7;
        input[1] += 8;
        input[4] += 3;
        input[2] += 5;
        input[4] += 4;
        input[0] += 10;
        input[1] += 12;
        input[3] += 6;
        input[4] += 5;
        input[2] += 7;
        input[0] += 13;
        input[4] += 6;
        input[2] += 9;
        input[0] += 15;
        input[3] += 8;
        input[4] += 7;
        input[2] += 11;
        input[1] += 16;
        input[4] += 8;
        
    }
    
    check5_sums(input, label);
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

// fully optimized - loops disappear
template <typename T, T *gInput>
void test_array5_global0(const std::string &label) {
  
    start_timer();
  
    T temp0 = gInput[0];
    T temp1 = gInput[1];
    T temp2 = gInput[2];
    T temp3 = gInput[3];
    T temp4 = gInput[4];
    
    temp0 += (T)52 * (T)iterations;
    temp1 += (T)40 * (T)iterations;
    temp2 += (T)36 * (T)iterations;
    temp3 += (T)20 * (T)iterations;
    temp4 += (T)36 * (T)iterations;

    gInput[0] = temp0;
    gInput[1] = temp1;
    gInput[2] = temp2;
    gInput[3] = temp3;
    gInput[4] = temp4;

    check5_sums(gInput, label);
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

// mostly optimized - scalar replacement, collapse terms, but still has a loop
template <typename T, T *gInput>
void test_array5_global1(const std::string &label) {
    int i;
    
    start_timer();
  
    T temp0 = gInput[0];
    T temp1 = gInput[1];
    T temp2 = gInput[2];
    T temp3 = gInput[3];
    T temp4 = gInput[4];
    
    for(i = 0; i < iterations; ++i) {
        temp0 += 52;
        temp1 += 40;
        temp2 += 36;
        temp3 += 20;
        temp4 += 36;
    }

    gInput[0] = temp0;
    gInput[1] = temp1;
    gInput[2] = temp2;
    gInput[3] = temp3;
    gInput[4] = temp4;

    check5_sums(gInput, label);
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

// barely optimized - scalar replacement, terms uncollapsed, still has a loop
template <typename T, T *gInput>
void test_array5_global2(const std::string &label) {
    int i;
    
    start_timer();
  
    T temp0 = gInput[0];
    T temp1 = gInput[1];
    T temp2 = gInput[2];
    T temp3 = gInput[3];
    T temp4 = gInput[4];
    
    for(i = 0; i < iterations; ++i) {
        temp0 += 2;
        temp2 += 1;
        temp4 += 1;
        temp1 += 4;
        temp3 += 2;
        temp0 += 5;
        temp2 += 3;
        temp4 += 2;
        temp3 += 4;
        temp0 += 7;
        temp1 += 8;
        temp4 += 3;
        temp2 += 5;
        temp4 += 4;
        temp0 += 10;
        temp1 += 12;
        temp3 += 6;
        temp4 += 5;
        temp2 += 7;
        temp0 += 13;
        temp4 += 6;
        temp2 += 9;
        temp0 += 15;
        temp3 += 8;
        temp4 += 7;
        temp2 += 11;
        temp1 += 16;
        temp4 += 8;
    }

    gInput[0] = temp0;
    gInput[1] = temp1;
    gInput[2] = temp2;
    gInput[3] = temp3;
    gInput[4] = temp4;

    check5_sums(gInput, label);
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

template <typename T, T *gInput>
void test_array5_global(const std::string &label) {
    int i;
  
    start_timer();
  
    for(i = 0; i < iterations; ++i) {
        
        gInput[0] += 2;
        gInput[2] += 1;
        gInput[4] += 1;
        gInput[1] += 4;
        gInput[3] += 2;
        gInput[0] += 5;
        gInput[2] += 3;
        gInput[4] += 2;
        gInput[3] += 4;
        gInput[0] += 7;
        gInput[1] += 8;
        gInput[4] += 3;
        gInput[2] += 5;
        gInput[4] += 4;
        gInput[0] += 10;
        gInput[1] += 12;
        gInput[3] += 6;
        gInput[4] += 5;
        gInput[2] += 7;
        gInput[0] += 13;
        gInput[4] += 6;
        gInput[2] += 9;
        gInput[0] += 15;
        gInput[3] += 8;
        gInput[4] += 7;
        gInput[2] += 11;
        gInput[1] += 16;
        gInput[4] += 8;
        
    }

    check5_sums(gInput, label);
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/
/******************************************************************************/

template< typename T >
void TestOneType()
{
    T data[SIZE];
    
    std::string myTypeName( getTypeName<T>() );
    
    gLabels.clear();

    ::fill(data, data+SIZE, T(init_value));
    test_array11_arg0(data, myTypeName + " scalar replacement of arrays argument opt");
    ::fill(data, data+SIZE, T(init_value));
    test_array11_arg1(data, myTypeName + " scalar replacement of arrays argument opt1");
    ::fill(data, data+SIZE, T(init_value));
    test_array11_arg1(data, myTypeName + " scalar replacement of arrays argument opt2");
    ::fill(data, data+SIZE, T(init_value));
    test_array11_arg(data, myTypeName + " scalar replacement of arrays argument");

    ::fill(data, data+SIZE, T(init_value));
    test_array5_arg0(data, myTypeName + " scalar replacement of small arrays argument opt");
    ::fill(data, data+SIZE, T(init_value));
    test_array5_arg1(data, myTypeName + " scalar replacement of small arrays argument opt1");
    ::fill(data, data+SIZE, T(init_value));
    test_array5_arg1(data, myTypeName + " scalar replacement of small arrays argument opt2");
    ::fill(data, data+SIZE, T(init_value));
    test_array5_arg(data, myTypeName + " scalar replacement of small arrays argument");

    test_array11_local0<T>(myTypeName + " scalar replacement of arrays local opt");
    test_array11_local1<T>(myTypeName + " scalar replacement of arrays local opt1");
    test_array11_local2<T>(myTypeName + " scalar replacement of arrays local opt2");
    test_array11_local<T> (myTypeName + " scalar replacement of arrays local");

    test_array5_local0<T>(myTypeName + " scalar replacement of small arrays local opt");
    test_array5_local1<T>(myTypeName + " scalar replacement of small arrays local opt1");
    test_array5_local2<T>(myTypeName + " scalar replacement of small arrays local opt2");
    test_array5_local<T> (myTypeName + " scalar replacement of small arrays local");
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
    

    
    TestOneType<int16_t>();

    ::fill(data16, data16+SIZE, int16_t(init_value));
    test_array11_global0<int16_t,data16>("int16_t scalar replacement of arrays global opt");
    ::fill(data16, data16+SIZE, int16_t(init_value));
    test_array11_global1<int16_t,data16>("int16_t scalar replacement of arrays global opt1");
    ::fill(data16, data16+SIZE, int16_t(init_value));
    test_array11_global2<int16_t,data16>("int16_t scalar replacement of arrays global opt2");
    ::fill(data16, data16+SIZE, int16_t(init_value));
    test_array11_global<int16_t,data16>("int16_t scalar replacement of arrays global");

    ::fill(data16, data16+SIZE, int16_t(init_value));
    test_array5_global0<int16_t,data16>("int16_t scalar replacement of small arrays global opt");
    ::fill(data16, data16+SIZE, int16_t(init_value));
    test_array5_global1<int16_t,data16>("int16_t scalar replacement of small arrays global opt1");
    ::fill(data16, data16+SIZE, int16_t(init_value));
    test_array5_global2<int16_t,data16>("int16_t scalar replacement of small arrays global opt2");
    ::fill(data16, data16+SIZE, int16_t(init_value));
    test_array5_global<int16_t,data16>("int16_t scalar replacement of small arrays global");
    
    summarize("int16_t scalar replacement of arrays", SIZE, iterations, kDontShowGMeans, kDontShowPenalty );
    
    
    TestOneType<int32_t>();

    ::fill(data32, data32+SIZE, int32_t(init_value));
    test_array11_global0<int32_t,data32>("int32_t scalar replacement of arrays global opt");
    ::fill(data32, data32+SIZE, int32_t(init_value));
    test_array11_global1<int32_t,data32>("int32_t scalar replacement of arrays global opt1");
    ::fill(data32, data32+SIZE, int32_t(init_value));
    test_array11_global2<int32_t,data32>("int32_t scalar replacement of arrays global opt2");
    ::fill(data32, data32+SIZE, int32_t(init_value));
    test_array11_global<int32_t,data32>("int32_t scalar replacement of arrays global");

    ::fill(data32, data32+SIZE, int32_t(init_value));
    test_array5_global0<int32_t,data32>("int32_t scalar replacement of small arrays global opt");
    ::fill(data32, data32+SIZE, int32_t(init_value));
    test_array5_global1<int32_t,data32>("int32_t scalar replacement of small arrays global opt1");
    ::fill(data32, data32+SIZE, int32_t(init_value));
    test_array5_global2<int32_t,data32>("int32_t scalar replacement of small arrays global opt2");
    ::fill(data32, data32+SIZE, int32_t(init_value));
    test_array5_global<int32_t,data32>("int32_t scalar replacement of small arrays global");
    
    summarize("int32_t scalar replacement of arrays", SIZE, iterations, kDontShowGMeans, kDontShowPenalty );
    
    
    TestOneType<uint64_t>();

    ::fill(data64, data64+SIZE, uint64_t(init_value));
    test_array11_global0<uint64_t,data64>("uint64_t scalar replacement of arrays global opt");
    ::fill(data64, data64+SIZE, uint64_t(init_value));
    test_array11_global1<uint64_t,data64>("uint64_t scalar replacement of arrays global opt1");
    ::fill(data64, data64+SIZE, uint64_t(init_value));
    test_array11_global2<uint64_t,data64>("uint64_t scalar replacement of arrays global opt2");
    ::fill(data64, data64+SIZE, uint64_t(init_value));
    test_array11_global<uint64_t,data64>("uint64_t scalar replacement of arrays global");

    ::fill(data64, data64+SIZE, uint64_t(init_value));
    test_array5_global0<uint64_t,data64>("uint64_t scalar replacement of small arrays global opt");
    ::fill(data64, data64+SIZE, uint64_t(init_value));
    test_array5_global1<uint64_t,data64>("uint64_t scalar replacement of small arrays global opt1");
    ::fill(data64, data64+SIZE, uint64_t(init_value));
    test_array5_global2<uint64_t,data64>("uint64_t scalar replacement of small arrays global opt2");
    ::fill(data64, data64+SIZE, uint64_t(init_value));
    test_array5_global<uint64_t,data64>("uint64_t scalar replacement of small arrays global");
    
    summarize("uint64_t scalar replacement of arrays", SIZE, iterations, kDontShowGMeans, kDontShowPenalty );




    // float does not have enough precision to accumulate the values and compare correctly
    // integers just overflow and compare exactly


    TestOneType<double>();
    
    ::fill(dataDouble, dataDouble+SIZE, double(init_value));
    test_array11_global0<double,dataDouble>("double scalar replacement of arrays global opt");
    ::fill(dataDouble, dataDouble+SIZE, double(init_value));
    test_array11_global1<double,dataDouble>("double scalar replacement of arrays global opt1");
    ::fill(dataDouble, dataDouble+SIZE, double(init_value));
    test_array11_global2<double,dataDouble>("double scalar replacement of arrays global opt2");
    ::fill(dataDouble, dataDouble+SIZE, double(init_value));
    test_array11_global<double,dataDouble>("double scalar replacement of arrays global");
    
    ::fill(dataDouble, dataDouble+SIZE, double(init_value));
    test_array5_global0<double,dataDouble>("double scalar replacement of small arrays global opt");
    ::fill(dataDouble, dataDouble+SIZE, double(init_value));
    test_array5_global1<double,dataDouble>("double scalar replacement of small arrays global opt1");
    ::fill(dataDouble, dataDouble+SIZE, double(init_value));
    test_array5_global2<double,dataDouble>("double scalar replacement of small arrays global opt2");
    ::fill(dataDouble, dataDouble+SIZE, double(init_value));
    test_array5_global<double,dataDouble>("double scalar replacement of small arrays global");
    
    summarize("double scalar replacement of arrays", SIZE, iterations, kDontShowGMeans, kDontShowPenalty );



    return 0;
}

// the end
/******************************************************************************/
/******************************************************************************/
