/*
    Copyright 2007-2008 Adobe Systems Incorporated
    Copyright 2018 Chris Cox
    Distributed under the MIT License (see accompanying file LICENSE_1_0_0.txt
    or a copy at http://stlab.adobe.com/licenses.html )


Goal: Test compiler optimizations related to scalar replacement of structure references.

Assumptions:

    1) The compiler will convert struct references to scalar calculations when necessary.
        input.val0 += 2;        ==>        input.val0 += 14;
        input.val0 += 5;
        input.val0 += 7;

    2) The compiler will do conversion (1) on local structs, struct arguments, and external structs.
    
    3) The compiler will apply further optimization to the resulting values.
        Best case: the loops disappear.



NOTE - Someone complained that they did not have enough registers to optimize this with 11 array values.
        But their compiler also fails at 5 array values (and 16 registers).

*/

/******************************************************************************/

#include "benchmark_stdint.hpp"
#include <stddef.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <math.h>
#include <deque>
#include <string>
#include "benchmark_results.h"
#include "benchmark_timer.h"
#include "benchmark_typenames.h"

/******************************************************************************/

// this constant may need to be adjusted to give reasonable minimum times
// For best results, times should be about 1.0 seconds for the minimum test run
// on 3Ghz desktop CPUs, 50M iterations is about 1.0 seconds
int iterations = 900000000;


#define SIZE     11


// initial value for filling our arrays, may be changed from the command line
int32_t init_value = 1;

/******************************************************************************/

std::deque<std::string> gLabels;

/******************************************************************************/

template<typename T>
struct test_struct {
    T value0;
    T value1;
    T value2;
    T value3;
    T value4;
    T value5;
    T value6;
    T value7;
    T value8;
    T value9;
    T value10;
};

// our global arrays of numbers to be operated upon

test_struct<int16_t> data16;

test_struct<int32_t> data32;

test_struct<uint64_t> data64;

test_struct<double> dataDouble;

/******************************************************************************/

template<typename T>
struct test_struct5 {
    T value0;
    T value1;
    T value2;
    T value3;
    T value4;
};

// our global arrays of numbers to be operated upon

test_struct5<int16_t> data16_5;

test_struct5<int32_t> data32_5;

test_struct5<uint64_t> data64_5;

test_struct5<double> dataDouble5;

/******************************************************************************/

template<typename T>
void fillstruct(test_struct<T> *sinput) {
    sinput->value0 = (T)init_value;
    sinput->value1 = (T)init_value;
    sinput->value2 = (T)init_value;
    sinput->value3 = (T)init_value;
    sinput->value4 = (T)init_value;
    sinput->value5 = (T)init_value;
    sinput->value6 = (T)init_value;
    sinput->value7 = (T)init_value;
    sinput->value8 = (T)init_value;
    sinput->value9 = (T)init_value;
    sinput->value10 = (T)init_value;
}

template<typename T>
void fillstruct5(test_struct5<T> *sinput) {
    sinput->value0 = (T)init_value;
    sinput->value1 = (T)init_value;
    sinput->value2 = (T)init_value;
    sinput->value3 = (T)init_value;
    sinput->value4 = (T)init_value;
}

/******************************************************************************/

template <typename T>
inline void check11_sums(test_struct<T> *sinput, const std::string &label ) {

    T temp0 = sinput->value0;
    T temp1 = sinput->value1;
    T temp2 = sinput->value2;
    T temp3 = sinput->value3;
    T temp4 = sinput->value4;
    T temp5 = sinput->value5;
    T temp6 = sinput->value6;
    T temp7 = sinput->value7;
    T temp8 = sinput->value8;
    T temp9 = sinput->value9;
    T temp10 = sinput->value10;
    
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

template <typename T>
void test_struct_arg0(test_struct<T> *sinput, const std::string &label) {
  
    start_timer();
    
    T temp0 = sinput->value0;
    T temp1 = sinput->value1;
    T temp2 = sinput->value2;
    T temp3 = sinput->value3;
    T temp4 = sinput->value4;
    T temp5 = sinput->value5;
    T temp6 = sinput->value6;
    T temp7 = sinput->value7;
    T temp8 = sinput->value8;
    T temp9 = sinput->value9;
    T temp10 = sinput->value10;
    
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
    
    sinput->value0 = temp0;
    sinput->value1 = temp1;
    sinput->value2 = temp2;
    sinput->value3 = temp3;
    sinput->value4 = temp4;
    sinput->value5 = temp5;
    sinput->value6 = temp6;
    sinput->value7 = temp7;
    sinput->value8 = temp8;
    sinput->value9 = temp9;
    sinput->value10 = temp10;
    
    check11_sums<T>(sinput, label);
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

// mostly optimized - scalar replacement, collapse terms, but still has a loop
template <typename T>
void test_struct_arg1(test_struct<T> *sinput, const std::string &label) {
    int i;
    
    start_timer();
    
    T temp0 = sinput->value0;
    T temp1 = sinput->value1;
    T temp2 = sinput->value2;
    T temp3 = sinput->value3;
    T temp4 = sinput->value4;
    T temp5 = sinput->value5;
    T temp6 = sinput->value6;
    T temp7 = sinput->value7;
    T temp8 = sinput->value8;
    T temp9 = sinput->value9;
    T temp10 = sinput->value10;
  
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
    
    sinput->value0 = temp0;
    sinput->value1 = temp1;
    sinput->value2 = temp2;
    sinput->value3 = temp3;
    sinput->value4 = temp4;
    sinput->value5 = temp5;
    sinput->value6 = temp6;
    sinput->value7 = temp7;
    sinput->value8 = temp8;
    sinput->value9 = temp9;
    sinput->value10 = temp10;
    
    check11_sums<T>(sinput, label);
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}


/******************************************************************************/

// barely optimized - basic scalar replacement, terms uncollapsed, still has a loop
template <typename T>
void test_struct_arg2(test_struct<T> *sinput, const std::string &label) {
    int i;
  
    start_timer();
    
    T temp0 = sinput->value0;
    T temp1 = sinput->value1;
    T temp2 = sinput->value2;
    T temp3 = sinput->value3;
    T temp4 = sinput->value4;
    T temp5 = sinput->value5;
    T temp6 = sinput->value6;
    T temp7 = sinput->value7;
    T temp8 = sinput->value8;
    T temp9 = sinput->value9;
    T temp10 = sinput->value10;
  
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
    
    sinput->value0 = temp0;
    sinput->value1 = temp1;
    sinput->value2 = temp2;
    sinput->value3 = temp3;
    sinput->value4 = temp4;
    sinput->value5 = temp5;
    sinput->value6 = temp6;
    sinput->value7 = temp7;
    sinput->value8 = temp8;
    sinput->value9 = temp9;
    sinput->value10 = temp10;
    
    check11_sums<T>(sinput, label);
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

// unoptimized
template <typename T>
void test_struct_arg(test_struct<T> *sinput, const std::string &label) {
    int i;
  
    start_timer();
  
    for(i = 0; i < iterations; ++i) {
        
        sinput->value8 += 12;
        sinput->value0 += 2;
        sinput->value8 += 6;
        sinput->value6 += 4;
        sinput->value10 += 3;
        sinput->value2 += 1;
        sinput->value4 += 1;
        sinput->value1 += 4;
        sinput->value3 += 2;
        sinput->value7 += 5;
        sinput->value8 += 5;
        sinput->value6 += 3;
        sinput->value0 += 5;
        sinput->value9 += 7;
        sinput->value2 += 3;
        sinput->value5 += 3;
        sinput->value4 += 2;
        sinput->value3 += 4;
        sinput->value6 += 2;
        sinput->value10 += 6;
        sinput->value0 += 7;
        sinput->value1 += 8;
        sinput->value4 += 3;
        sinput->value9 += 9;
        sinput->value2 += 5;
        sinput->value10 += 9;
        sinput->value8 += 11;
        sinput->value4 += 4;
        sinput->value6 += 1;
        sinput->value5 += 3;
        sinput->value0 += 10;
        sinput->value10 += 11;
        sinput->value8 += 9;
        sinput->value1 += 12;
        sinput->value9 += 11;
        sinput->value3 += 6;
        sinput->value4 += 5;
        sinput->value6 += 2;
        sinput->value2 += 7;
        sinput->value0 += 13;
        sinput->value9 += 13;
        sinput->value4 += 6;
        sinput->value10 += 14;
        sinput->value2 += 9;
        sinput->value8 += 4;
        sinput->value5 += 3;
        sinput->value0 += 15;
        sinput->value3 += 8;
        sinput->value7 += 10;
        sinput->value6 += 3;
        sinput->value4 += 7;
        sinput->value8 += 10;
        sinput->value2 += 11;
        sinput->value1 += 16;
        sinput->value4 += 8;
        sinput->value10 += 17;
        sinput->value7 += 15;
        sinput->value9 += 15;
        
    }
    
    check11_sums<T>(sinput, label);
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

template <typename T>
void test_struct_local0(const std::string &label) {
    
    test_struct<T> sinput;
    fillstruct<T>( &sinput );
  
    start_timer();
    
    T temp0 = sinput.value0;
    T temp1 = sinput.value1;
    T temp2 = sinput.value2;
    T temp3 = sinput.value3;
    T temp4 = sinput.value4;
    T temp5 = sinput.value5;
    T temp6 = sinput.value6;
    T temp7 = sinput.value7;
    T temp8 = sinput.value8;
    T temp9 = sinput.value9;
    T temp10 = sinput.value10;
    
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

    sinput.value0 = temp0;
    sinput.value1 = temp1;
    sinput.value2 = temp2;
    sinput.value3 = temp3;
    sinput.value4 = temp4;
    sinput.value5 = temp5;
    sinput.value6 = temp6;
    sinput.value7 = temp7;
    sinput.value8 = temp8;
    sinput.value9 = temp9;
    sinput.value10 = temp10;
    
    check11_sums<T>(&sinput, label);
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

// mostly optimized - scalar replacement, collapse terms, but still has a loop
template <typename T>
void test_struct_local1(const std::string &label) {
    int i;
    test_struct<T> sinput;
    
    fillstruct<T>( &sinput );
  
    start_timer();
    
    T temp0 = sinput.value0;
    T temp1 = sinput.value1;
    T temp2 = sinput.value2;
    T temp3 = sinput.value3;
    T temp4 = sinput.value4;
    T temp5 = sinput.value5;
    T temp6 = sinput.value6;
    T temp7 = sinput.value7;
    T temp8 = sinput.value8;
    T temp9 = sinput.value9;
    T temp10 = sinput.value10;
  
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

    sinput.value0 = temp0;
    sinput.value1 = temp1;
    sinput.value2 = temp2;
    sinput.value3 = temp3;
    sinput.value4 = temp4;
    sinput.value5 = temp5;
    sinput.value6 = temp6;
    sinput.value7 = temp7;
    sinput.value8 = temp8;
    sinput.value9 = temp9;
    sinput.value10 = temp10;
    
    check11_sums<T>(&sinput, label);
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

// barely optimized - basic scalar replacement, terms uncollapsed, still has a loop
template <typename T>
void test_struct_local2(const std::string &label) {
    int i;
    
    test_struct<T> sinput;
    fillstruct<T>( &sinput );
  
    start_timer();
    
    T temp0 = sinput.value0;
    T temp1 = sinput.value1;
    T temp2 = sinput.value2;
    T temp3 = sinput.value3;
    T temp4 = sinput.value4;
    T temp5 = sinput.value5;
    T temp6 = sinput.value6;
    T temp7 = sinput.value7;
    T temp8 = sinput.value8;
    T temp9 = sinput.value9;
    T temp10 = sinput.value10;
  
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

    sinput.value0 = temp0;
    sinput.value1 = temp1;
    sinput.value2 = temp2;
    sinput.value3 = temp3;
    sinput.value4 = temp4;
    sinput.value5 = temp5;
    sinput.value6 = temp6;
    sinput.value7 = temp7;
    sinput.value8 = temp8;
    sinput.value9 = temp9;
    sinput.value10 = temp10;
    
    check11_sums<T>(&sinput, label);
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}


/******************************************************************************/

// unoptimized
template <typename T>
void test_struct_local(const std::string &label) {
    int i;
    
    test_struct<T> sinput;
    fillstruct<T>( &sinput );
  
    start_timer();
  
    for(i = 0; i < iterations; ++i) {
        
        sinput.value8 += 12;
        sinput.value0 += 2;
        sinput.value8 += 6;
        sinput.value6 += 4;
        sinput.value10 += 3;
        sinput.value2 += 1;
        sinput.value4 += 1;
        sinput.value1 += 4;
        sinput.value3 += 2;
        sinput.value7 += 5;
        sinput.value8 += 5;
        sinput.value6 += 3;
        sinput.value0 += 5;
        sinput.value9 += 7;
        sinput.value2 += 3;
        sinput.value5 += 3;
        sinput.value4 += 2;
        sinput.value3 += 4;
        sinput.value6 += 2;
        sinput.value10 += 6;
        sinput.value0 += 7;
        sinput.value1 += 8;
        sinput.value4 += 3;
        sinput.value9 += 9;
        sinput.value2 += 5;
        sinput.value10 += 9;
        sinput.value8 += 11;
        sinput.value4 += 4;
        sinput.value6 += 1;
        sinput.value5 += 3;
        sinput.value0 += 10;
        sinput.value10 += 11;
        sinput.value8 += 9;
        sinput.value1 += 12;
        sinput.value9 += 11;
        sinput.value3 += 6;
        sinput.value4 += 5;
        sinput.value6 += 2;
        sinput.value2 += 7;
        sinput.value0 += 13;
        sinput.value9 += 13;
        sinput.value4 += 6;
        sinput.value10 += 14;
        sinput.value2 += 9;
        sinput.value8 += 4;
        sinput.value5 += 3;
        sinput.value0 += 15;
        sinput.value3 += 8;
        sinput.value7 += 10;
        sinput.value6 += 3;
        sinput.value4 += 7;
        sinput.value8 += 10;
        sinput.value2 += 11;
        sinput.value1 += 16;
        sinput.value4 += 8;
        sinput.value10 += 17;
        sinput.value7 += 15;
        sinput.value9 += 15;
        
    }
    
    check11_sums<T>(&sinput, label);
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/


template <typename T, typename S, S *gInput>
void test_struct_global0(const std::string &label) {
  
    start_timer();
    
    T temp0 = gInput->value0;
    T temp1 = gInput->value1;
    T temp2 = gInput->value2;
    T temp3 = gInput->value3;
    T temp4 = gInput->value4;
    T temp5 = gInput->value5;
    T temp6 = gInput->value6;
    T temp7 = gInput->value7;
    T temp8 = gInput->value8;
    T temp9 = gInput->value9;
    T temp10 = gInput->value10;
    
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

    gInput->value0 = temp0;
    gInput->value1 = temp1;
    gInput->value2 = temp2;
    gInput->value3 = temp3;
    gInput->value4 = temp4;
    gInput->value5 = temp5;
    gInput->value6 = temp6;
    gInput->value7 = temp7;
    gInput->value8 = temp8;
    gInput->value9 = temp9;
    gInput->value10 = temp10;
        
    check11_sums<T>(gInput, label);
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

// mostly optimized - scalar replacement, collapse terms, but still has a loop
template <typename T, typename S, S *gInput>
void test_struct_global1(const std::string &label) {
    int i;
  
    start_timer();
    
    T temp0 = gInput->value0;
    T temp1 = gInput->value1;
    T temp2 = gInput->value2;
    T temp3 = gInput->value3;
    T temp4 = gInput->value4;
    T temp5 = gInput->value5;
    T temp6 = gInput->value6;
    T temp7 = gInput->value7;
    T temp8 = gInput->value8;
    T temp9 = gInput->value9;
    T temp10 = gInput->value10;
  
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

    gInput->value0 = temp0;
    gInput->value1 = temp1;
    gInput->value2 = temp2;
    gInput->value3 = temp3;
    gInput->value4 = temp4;
    gInput->value5 = temp5;
    gInput->value6 = temp6;
    gInput->value7 = temp7;
    gInput->value8 = temp8;
    gInput->value9 = temp9;
    gInput->value10 = temp10;
    
    check11_sums(gInput, label);
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}


/******************************************************************************/

// barely optimized - basic scalar replacement, terms uncollapsed, still has a loop
template <typename T, typename S, S *gInput>
void test_struct_global2(const std::string &label) {
    int i;
  
    start_timer();
    
    T temp0 = gInput->value0;
    T temp1 = gInput->value1;
    T temp2 = gInput->value2;
    T temp3 = gInput->value3;
    T temp4 = gInput->value4;
    T temp5 = gInput->value5;
    T temp6 = gInput->value6;
    T temp7 = gInput->value7;
    T temp8 = gInput->value8;
    T temp9 = gInput->value9;
    T temp10 = gInput->value10;
  
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

    gInput->value0 = temp0;
    gInput->value1 = temp1;
    gInput->value2 = temp2;
    gInput->value3 = temp3;
    gInput->value4 = temp4;
    gInput->value5 = temp5;
    gInput->value6 = temp6;
    gInput->value7 = temp7;
    gInput->value8 = temp8;
    gInput->value9 = temp9;
    gInput->value10 = temp10;
    
    check11_sums(gInput, label);
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

// unoptimized
template <typename T, typename S, S *gInput>
void test_struct_global(const std::string &label) {
    int i;
  
    start_timer();
  
    for(i = 0; i < iterations; ++i) {
        
        gInput->value8 += 12;
        gInput->value0 += 2;
        gInput->value8 += 6;
        gInput->value6 += 4;
        gInput->value10 += 3;
        gInput->value2 += 1;
        gInput->value4 += 1;
        gInput->value1 += 4;
        gInput->value3 += 2;
        gInput->value7 += 5;
        gInput->value8 += 5;
        gInput->value6 += 3;
        gInput->value0 += 5;
        gInput->value9 += 7;
        gInput->value2 += 3;
        gInput->value5 += 3;
        gInput->value4 += 2;
        gInput->value3 += 4;
        gInput->value6 += 2;
        gInput->value10 += 6;
        gInput->value0 += 7;
        gInput->value1 += 8;
        gInput->value4 += 3;
        gInput->value9 += 9;
        gInput->value2 += 5;
        gInput->value10 += 9;
        gInput->value8 += 11;
        gInput->value4 += 4;
        gInput->value6 += 1;
        gInput->value5 += 3;
        gInput->value0 += 10;
        gInput->value10 += 11;
        gInput->value8 += 9;
        gInput->value1 += 12;
        gInput->value9 += 11;
        gInput->value3 += 6;
        gInput->value4 += 5;
        gInput->value6 += 2;
        gInput->value2 += 7;
        gInput->value0 += 13;
        gInput->value9 += 13;
        gInput->value4 += 6;
        gInput->value10 += 14;
        gInput->value2 += 9;
        gInput->value8 += 4;
        gInput->value5 += 3;
        gInput->value0 += 15;
        gInput->value3 += 8;
        gInput->value7 += 10;
        gInput->value6 += 3;
        gInput->value4 += 7;
        gInput->value8 += 10;
        gInput->value2 += 11;
        gInput->value1 += 16;
        gInput->value4 += 8;
        gInput->value10 += 17;
        gInput->value7 += 15;
        gInput->value9 += 15;
        
    }
    
    check11_sums(gInput, label);
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/
/******************************************************************************/

template <typename T>
inline void check5_sums(test_struct5<T> *sinput, const std::string &label ) {

    T temp0 = sinput->value0;
    T temp1 = sinput->value1;
    T temp2 = sinput->value2;
    T temp3 = sinput->value3;
    T temp4 = sinput->value4;
    
    T temp0_base = (T)init_value + (T)iterations * 52;
    T temp1_base = (T)init_value + (T)iterations * 40;
    T temp2_base = (T)init_value + (T)iterations * 36;
    T temp3_base = (T)init_value + (T)iterations * 20;
    T temp4_base = (T)init_value + (T)iterations * 36;

    if (   temp0 != temp0_base
        || temp1 != temp1_base
        || temp2 != temp2_base
        || temp3 != temp3_base
        || temp4 != temp4_base )
        printf("test %s failed\n", label.c_str() );
}

/******************************************************************************/

template <typename T>
void test_struct5_arg0(test_struct5<T> *sinput, const std::string &label) {
  
    start_timer();
    
    T temp0 = sinput->value0;
    T temp1 = sinput->value1;
    T temp2 = sinput->value2;
    T temp3 = sinput->value3;
    T temp4 = sinput->value4;
    
    temp0 += (T)52 * (T)iterations;
    temp1 += (T)40 * (T)iterations;
    temp2 += (T)36 * (T)iterations;
    temp3 += (T)20 * (T)iterations;
    temp4 += (T)36 * (T)iterations;
    
    sinput->value0 = temp0;
    sinput->value1 = temp1;
    sinput->value2 = temp2;
    sinput->value3 = temp3;
    sinput->value4 = temp4;
    
    check5_sums<T>(sinput, label);
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

// mostly optimized - scalar replacement, collapse terms, but still has a loop
template <typename T>
void test_struct5_arg1(test_struct5<T> *sinput, const std::string &label) {
    int i;
    
    start_timer();
    
    T temp0 = sinput->value0;
    T temp1 = sinput->value1;
    T temp2 = sinput->value2;
    T temp3 = sinput->value3;
    T temp4 = sinput->value4;
  
    for(i = 0; i < iterations; ++i) {
        temp0 += 52;
        temp1 += 40;
        temp2 += 36;
        temp3 += 20;
        temp4 += 36;
    }
    
    sinput->value0 = temp0;
    sinput->value1 = temp1;
    sinput->value2 = temp2;
    sinput->value3 = temp3;
    sinput->value4 = temp4;
    
    check5_sums<T>(sinput, label);
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}


/******************************************************************************/

// barely optimized - basic scalar replacement, terms uncollapsed, still has a loop
template <typename T>
void test_struct5_arg2(test_struct5<T> *sinput, const std::string &label) {
    int i;
  
    start_timer();
    
    T temp0 = sinput->value0;
    T temp1 = sinput->value1;
    T temp2 = sinput->value2;
    T temp3 = sinput->value3;
    T temp4 = sinput->value4;
  
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
    
    sinput->value0 = temp0;
    sinput->value1 = temp1;
    sinput->value2 = temp2;
    sinput->value3 = temp3;
    sinput->value4 = temp4;
    
    check5_sums<T>(sinput, label);
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

// unoptimized
template <typename T>
void test_struct5_arg(test_struct5<T> *sinput, const std::string &label) {
    int i;
  
    start_timer();
  
    for(i = 0; i < iterations; ++i) {
        
        sinput->value0 += 2;
        sinput->value2 += 1;
        sinput->value4 += 1;
        sinput->value1 += 4;
        sinput->value3 += 2;
        sinput->value0 += 5;
        sinput->value2 += 3;
        sinput->value4 += 2;
        sinput->value3 += 4;
        sinput->value0 += 7;
        sinput->value1 += 8;
        sinput->value4 += 3;
        sinput->value2 += 5;
        sinput->value4 += 4;
        sinput->value0 += 10;
        sinput->value1 += 12;
        sinput->value3 += 6;
        sinput->value4 += 5;
        sinput->value2 += 7;
        sinput->value0 += 13;
        sinput->value4 += 6;
        sinput->value2 += 9;
        sinput->value0 += 15;
        sinput->value3 += 8;
        sinput->value4 += 7;
        sinput->value2 += 11;
        sinput->value1 += 16;
        sinput->value4 += 8;
        
    }
    
    check5_sums<T>(sinput, label);
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

template <typename T>
void test_struct5_local0(const std::string &label) {
    
    test_struct5<T> sinput;
    fillstruct5<T>( &sinput );
  
    start_timer();
    
    T temp0 = sinput.value0;
    T temp1 = sinput.value1;
    T temp2 = sinput.value2;
    T temp3 = sinput.value3;
    T temp4 = sinput.value4;
    
    temp0 += (T)52 * (T)iterations;
    temp1 += (T)40 * (T)iterations;
    temp2 += (T)36 * (T)iterations;
    temp3 += (T)20 * (T)iterations;
    temp4 += (T)36 * (T)iterations;

    sinput.value0 = temp0;
    sinput.value1 = temp1;
    sinput.value2 = temp2;
    sinput.value3 = temp3;
    sinput.value4 = temp4;
    
    check5_sums<T>(&sinput, label);
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

// mostly optimized - scalar replacement, collapse terms, but still has a loop
template <typename T>
void test_struct5_local1(const std::string &label) {
    int i;
    test_struct5<T> sinput;
    
    fillstruct5<T>( &sinput );
  
    start_timer();
    
    T temp0 = sinput.value0;
    T temp1 = sinput.value1;
    T temp2 = sinput.value2;
    T temp3 = sinput.value3;
    T temp4 = sinput.value4;
  
    for(i = 0; i < iterations; ++i) {
        temp0 += 52;
        temp1 += 40;
        temp2 += 36;
        temp3 += 20;
        temp4 += 36;
    }

    sinput.value0 = temp0;
    sinput.value1 = temp1;
    sinput.value2 = temp2;
    sinput.value3 = temp3;
    sinput.value4 = temp4;
    
    check5_sums<T>(&sinput, label);
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

// barely optimized - basic scalar replacement, terms uncollapsed, still has a loop
template <typename T>
void test_struct5_local2(const std::string &label) {
    int i;
    
    test_struct5<T> sinput;
    fillstruct5<T>( &sinput );
  
    start_timer();
    
    T temp0 = sinput.value0;
    T temp1 = sinput.value1;
    T temp2 = sinput.value2;
    T temp3 = sinput.value3;
    T temp4 = sinput.value4;
  
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

    sinput.value0 = temp0;
    sinput.value1 = temp1;
    sinput.value2 = temp2;
    sinput.value3 = temp3;
    sinput.value4 = temp4;
    
    check5_sums<T>(&sinput, label);
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}


/******************************************************************************/

// unoptimized
template <typename T>
void test_struct5_local(const std::string &label) {
    int i;
    
    test_struct5<T> sinput;
    fillstruct5<T>( &sinput );
  
    start_timer();
  
    for(i = 0; i < iterations; ++i) {
        
        sinput.value0 += 2;
        sinput.value2 += 1;
        sinput.value4 += 1;
        sinput.value1 += 4;
        sinput.value3 += 2;
        sinput.value0 += 5;
        sinput.value2 += 3;
        sinput.value4 += 2;
        sinput.value3 += 4;
        sinput.value0 += 7;
        sinput.value1 += 8;
        sinput.value4 += 3;
        sinput.value2 += 5;
        sinput.value4 += 4;
        sinput.value0 += 10;
        sinput.value1 += 12;
        sinput.value3 += 6;
        sinput.value4 += 5;
        sinput.value2 += 7;
        sinput.value0 += 13;
        sinput.value4 += 6;
        sinput.value2 += 9;
        sinput.value0 += 15;
        sinput.value3 += 8;
        sinput.value4 += 7;
        sinput.value2 += 11;
        sinput.value1 += 16;
        sinput.value4 += 8;
        
    }
    
    check5_sums<T>(&sinput, label);
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/


template <typename T, typename S, S *gInput>
void test_struct5_global0(const std::string &label) {
  
    start_timer();
    
    T temp0 = gInput->value0;
    T temp1 = gInput->value1;
    T temp2 = gInput->value2;
    T temp3 = gInput->value3;
    T temp4 = gInput->value4;
    
    temp0 += (T)52 * (T)iterations;
    temp1 += (T)40 * (T)iterations;
    temp2 += (T)36 * (T)iterations;
    temp3 += (T)20 * (T)iterations;
    temp4 += (T)36 * (T)iterations;

    gInput->value0 = temp0;
    gInput->value1 = temp1;
    gInput->value2 = temp2;
    gInput->value3 = temp3;
    gInput->value4 = temp4;
    
    check5_sums<T>(gInput, label);
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

// mostly optimized - scalar replacement, collapse terms, but still has a loop
template <typename T, typename S, S *gInput>
void test_struct5_global1(const std::string &label) {
    int i;
  
    start_timer();
    
    T temp0 = gInput->value0;
    T temp1 = gInput->value1;
    T temp2 = gInput->value2;
    T temp3 = gInput->value3;
    T temp4 = gInput->value4;
  
    for(i = 0; i < iterations; ++i) {
        temp0 += 52;
        temp1 += 40;
        temp2 += 36;
        temp3 += 20;
        temp4 += 36;
    }

    gInput->value0 = temp0;
    gInput->value1 = temp1;
    gInput->value2 = temp2;
    gInput->value3 = temp3;
    gInput->value4 = temp4;
    
    check5_sums(gInput, label);
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}


/******************************************************************************/

// barely optimized - basic scalar replacement, terms uncollapsed, still has a loop
template <typename T, typename S, S *gInput>
void test_struct5_global2(const std::string &label) {
    int i;
  
    start_timer();
    
    T temp0 = gInput->value0;
    T temp1 = gInput->value1;
    T temp2 = gInput->value2;
    T temp3 = gInput->value3;
    T temp4 = gInput->value4;
  
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

    gInput->value0 = temp0;
    gInput->value1 = temp1;
    gInput->value2 = temp2;
    gInput->value3 = temp3;
    gInput->value4 = temp4;
    
    check5_sums(gInput, label);
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

// unoptimized
template <typename T, typename S, S *gInput>
void test_struct5_global(const std::string &label) {
    int i;
  
    start_timer();
  
    for(i = 0; i < iterations; ++i) {
        
        gInput->value0 += 2;
        gInput->value2 += 1;
        gInput->value4 += 1;
        gInput->value1 += 4;
        gInput->value3 += 2;
        gInput->value0 += 5;
        gInput->value2 += 3;
        gInput->value4 += 2;
        gInput->value3 += 4;
        gInput->value0 += 7;
        gInput->value1 += 8;
        gInput->value4 += 3;
        gInput->value2 += 5;
        gInput->value4 += 4;
        gInput->value0 += 10;
        gInput->value1 += 12;
        gInput->value3 += 6;
        gInput->value4 += 5;
        gInput->value2 += 7;
        gInput->value0 += 13;
        gInput->value4 += 6;
        gInput->value2 += 9;
        gInput->value0 += 15;
        gInput->value3 += 8;
        gInput->value4 += 7;
        gInput->value2 += 11;
        gInput->value1 += 16;
        gInput->value4 += 8;
        
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
    test_struct<T> dataa;
    test_struct5<T> dataa_5;
    
    std::string myTypeName( getTypeName<T>() );
    
    gLabels.clear();

    fillstruct<T>( &dataa );
    test_struct_arg0(&dataa, myTypeName + " scalar replacement of structs argument opt");
    fillstruct<T>( &dataa );
    test_struct_arg1(&dataa, myTypeName + " scalar replacement of structs argument opt1");
    fillstruct<T>( &dataa );
    test_struct_arg2(&dataa, myTypeName + " scalar replacement of structs argument opt2");
    fillstruct<T>( &dataa );
    test_struct_arg(&dataa, myTypeName + " scalar replacement of structs argument");
    
    fillstruct5<T>( &dataa_5 );
    test_struct5_arg0(&dataa_5, myTypeName + " scalar replacement of small structs argument opt");
    fillstruct5<T>( &dataa_5 );
    test_struct5_arg1(&dataa_5, myTypeName + " calar replacement of small structs argument opt1");
    fillstruct5<T>( &dataa_5 );
    test_struct5_arg2(&dataa_5, myTypeName + " scalar replacement of small structs argument opt2");
    fillstruct5<T>( &dataa_5 );
    test_struct5_arg(&dataa_5, myTypeName + " scalar replacement of small structs argument");
    
    test_struct_local0<T>(myTypeName + " scalar replacement of structs local opt");
    test_struct_local1<T>(myTypeName + " scalar replacement of structs local opt1");
    test_struct_local2<T>(myTypeName + " scalar replacement of structs local opt2");
    test_struct_local<T>(myTypeName + " scalar replacement of structs local");
    
    test_struct5_local0<T>(myTypeName + " scalar replacement of small structs local opt");
    test_struct5_local1<T>(myTypeName + " scalar replacement of small structs local opt1");
    test_struct5_local2<T>(myTypeName + " scalar replacement of small structs local opt2");
    test_struct5_local<T>(myTypeName + " scalar replacement of small structs local");
}


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

    fillstruct<int16_t>( &data16 );
    test_struct_global0<int16_t, test_struct<int16_t>, &data16>("int16_t scalar replacement of structs global opt");
    fillstruct<int16_t>( &data16 );
    test_struct_global1<int16_t, test_struct<int16_t>, &data16>("int16_t scalar replacement of structs global opt1");
    fillstruct<int16_t>( &data16 );
    test_struct_global2<int16_t, test_struct<int16_t>, &data16>("int16_t scalar replacement of structs global opt2");
    fillstruct<int16_t>( &data16 );
    test_struct_global<int16_t, test_struct<int16_t>, &data16>("int16_t scalar replacement of structs global");

    fillstruct5<int16_t>( &data16_5 );
    test_struct5_global0<int16_t, test_struct5<int16_t>, &data16_5>("int16_t scalar replacement of small structs global opt");
    fillstruct5<int16_t>( &data16_5 );
    test_struct5_global1<int16_t, test_struct5<int16_t>, &data16_5>("int16_t scalar replacement of small structs global opt1");
    fillstruct5<int16_t>( &data16_5 );
    test_struct5_global2<int16_t, test_struct5<int16_t>, &data16_5>("int16_t scalar replacement of small structs global opt2");
    fillstruct5<int16_t>( &data16_5 );
    test_struct5_global<int16_t, test_struct5<int16_t>, &data16_5>("int16_t scalar replacement of small structs global");

    summarize("int16_t scalar replacement of structs", SIZE, iterations, kDontShowGMeans, kDontShowPenalty );
    
    
    

    TestOneType<int32_t>();

    fillstruct<int32_t>( &data32 );
    test_struct_global0<int32_t, test_struct<int32_t>, &data32>("int32_t scalar replacement of structs global opt");
    fillstruct<int32_t>( &data32 );
    test_struct_global1<int32_t, test_struct<int32_t>, &data32>("int32_t scalar replacement of structs global opt1");
    fillstruct<int32_t>( &data32 );
    test_struct_global2<int32_t, test_struct<int32_t>, &data32>("int32_t scalar replacement of structs global opt2");
    fillstruct<int32_t>( &data32 );
    test_struct_global<int32_t, test_struct<int32_t>, &data32>("int32_t scalar replacement of structs global");

    fillstruct5<int32_t>( &data32_5 );
    test_struct5_global0<int32_t, test_struct5<int32_t>, &data32_5>("int32_t scalar replacement of small structs global opt");
    fillstruct5<int32_t>( &data32_5 );
    test_struct5_global1<int32_t, test_struct5<int32_t>, &data32_5>("int32_t scalar replacement of small structs global opt1");
    fillstruct5<int32_t>( &data32_5 );
    test_struct5_global2<int32_t, test_struct5<int32_t>, &data32_5>("int32_t scalar replacement of small structs global opt2");
    fillstruct5<int32_t>( &data32_5 );
    test_struct5_global<int32_t, test_struct5<int32_t>, &data32_5>("int32_t scalar replacement of small structs global");

    summarize("int32_t scalar replacement of structs", SIZE, iterations, kDontShowGMeans, kDontShowPenalty );



    TestOneType<uint64_t>();

    fillstruct<uint64_t>( &data64 );
    test_struct_global0<uint64_t, test_struct<uint64_t>, &data64>("uint64_t scalar replacement of structs global opt");
    fillstruct<uint64_t>( &data64 );
    test_struct_global1<uint64_t, test_struct<uint64_t>, &data64>("uint64_t scalar replacement of structs global opt1");
    fillstruct<uint64_t>( &data64 );
    test_struct_global2<uint64_t, test_struct<uint64_t>, &data64>("uint64_t scalar replacement of structs global opt2");
    fillstruct<uint64_t>( &data64 );
    test_struct_global<uint64_t, test_struct<uint64_t>, &data64>("uint64_t scalar replacement of structs global");

    fillstruct5<uint64_t>( &data64_5 );
    test_struct5_global0<uint64_t, test_struct5<uint64_t>, &data64_5>("uint64_t scalar replacement of small structs global opt");
    fillstruct5<uint64_t>( &data64_5 );
    test_struct5_global1<uint64_t, test_struct5<uint64_t>, &data64_5>("uint64_t scalar replacement of small structs global opt1");
    fillstruct5<uint64_t>( &data64_5 );
    test_struct5_global2<uint64_t, test_struct5<uint64_t>, &data64_5>("uint64_t scalar replacement of small structs global opt2");
    fillstruct5<uint64_t>( &data64_5 );
    test_struct5_global<uint64_t, test_struct5<uint64_t>, &data64_5>("uint64_t scalar replacement of small structs global");

    summarize("uint64_t scalar replacement of structs", SIZE, iterations, kDontShowGMeans, kDontShowPenalty );



    // float does not have enough precision to accumulate the values and compare correctly
    // integers just overflow and compare exactly
    

    TestOneType<double>();

    fillstruct<double>( &dataDouble );
    test_struct_global0<double, test_struct<double>, &dataDouble>("double scalar replacement of structs global opt");
    fillstruct<double>( &dataDouble );
    test_struct_global1<double, test_struct<double>, &dataDouble>("double scalar replacement of structs global opt1");
    fillstruct<double>( &dataDouble );
    test_struct_global2<double, test_struct<double>, &dataDouble>("double scalar replacement of structs global opt2");
    fillstruct<double>( &dataDouble );
    test_struct_global<double, test_struct<double>, &dataDouble>("double scalar replacement of structs global");

    fillstruct5<double>( &dataDouble5 );
    test_struct5_global0<double, test_struct5<double>, &dataDouble5>("double scalar replacement of small structs global opt");
    fillstruct5<double>( &dataDouble5 );
    test_struct5_global1<double, test_struct5<double>, &dataDouble5>("double scalar replacement of small structs global opt1");
    fillstruct5<double>( &dataDouble5 );
    test_struct5_global2<double, test_struct5<double>, &dataDouble5>("double scalar replacement of small structs global opt2");
    fillstruct5<double>( &dataDouble5 );
    test_struct5_global<double, test_struct5<double>, &dataDouble5>("double scalar replacement of small structs global");

    summarize("scalar replacement of structs double", SIZE, iterations, kDontShowGMeans, kDontShowPenalty );


    return 0;
}

// the end
/******************************************************************************/
/******************************************************************************/
