/*
    Copyright 2007-2008 Adobe Systems Incorporated
    Copyright 2018-2019 Chris Cox
    Distributed under the MIT License (see accompanying file LICENSE_1_0_0.txt
    or a copy at http://stlab.adobe.com/licenses.html )


Goal: Test common sub expression (CSE) optimization with simple language defined types.


Assumptions:

    1) The compiler will apply common subexpression elimination on simple types.
        result1 = input1 + A * (input1 - input2)          temp = A * (input1 - input2)
        result2 = input2 + A * (input1 - input2)    ==>   result1 = input1 + temp;
                                                          result2 = input2 + temp;

    2) The CSE optimizations will recognize symmetrical expressions with flipped argument order.
        result1 = input1 + A * (input1 + input2)          temp = A * (input1 + input2)
        result2 = input2 + A * (input2 + input1)    ==>   result1 = input1 + temp;
                                                          result2 = input2 + temp;

    3) The CSE optimzations won't break if the number of expressions used increases.

    4) The CSE optimizations won't be affected by template or inline usage.

    5) Further optimizations will be applied after CSE (algebraic simplification, vectorization, etc.).


Current compilers do OK on simple tests, but fail when the code gets more complicated.
We need to spend time getting the basics working well in our compilers.





NOTE - declaring test function arguments const, so far has no effect on performance results.


TODO - move most of this test code into shared tests so I can test custom types
TODO - logic ops for integer types ( | & ^ >> << )
    need a good way to separate int and float tests without duplicating code, or generating errors!
TODO - does the compiler recognize builtin functions (sin, log, abs) as returning a const value and subject to CSE?
    floating point rules may hinder further optimization
    but cos(x) + cos(x) + cos(x) + cos(x) should only call cos() once.

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
int iterations = 2500000;


// 8000 items, or between 8k and 64k of data
// this is intended to remain within the L2 cache of most common CPUs
#define SIZE     8000


// initial value for filling our arrays, may be changed from the command line
double init_value = 1.0;

/******************************************************************************/

#include "benchmark_shared_tests.h"

/******************************************************************************/

template <typename T, typename Shifter>
inline void check_shifted_variable_sum_CSE(T result, T var, const std::string &label) {
    T temp(0.0);
    if (!tolerance_equal<T>(result,temp))
        printf("test %s failed\n", label.c_str());
}

/******************************************************************************/

template <typename T, typename Shifter>
inline void check_shifted_variable_sum_CSE(T result, const std::string &label) {
    T temp(0.0);
    if (!tolerance_equal<T>(result,temp))
        printf("test %s failed\n", label.c_str());
}

/******************************************************************************/

std::deque<std::string> gLabels;

template <typename T, typename Shifter>
void test_CSE1_fullopt(T* first, int count, T v1, const std::string label) {
    start_timer();

    // This is about as far as most compilers can optmize the code.
    // So far none realize that all array entries have the same value and result is always zero.
    for(int i = 0; i < iterations; ++i) {
        T result(0);
        T temp2 = first[0] - first[1];
        result += temp2;
        for (int n = 1; n < count; ++n) {
            T temp1 = (first[n-1] - first[n]);
            result += temp1;
        }
        check_shifted_variable_sum_CSE<T, Shifter>(result, v1, label);
    }
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

template <typename T, typename Shifter>
void test_CSE1_halfopt(T* first, int count, T v1, const std::string label) {
    start_timer();

    // Just CSE, without algebraic simplification applied.
    for(int i = 0; i < iterations; ++i) {
        T result(0);
        T temp0 = Shifter::do_shift( v1, first[0], first[1] );
        temp0 += temp0;
        T temp2 = (first[0] + temp0) - (first[1] + temp0);  // algebraic simplification should eliminate temp0 and it's calculation
        result += temp2;
        for (int n = 1; n < count; ++n) {
            T temp = Shifter::do_shift( v1, first[n-1], first[n] );
            temp += temp;
            T temp1 = (first[n-1] + temp) - (first[n] + temp); // algebraic simplification should eliminate temp and it's calculation
            result += temp1;
        }
        check_shifted_variable_sum_CSE<T, Shifter>(result, v1, label);
    }

    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

template <typename T, typename Shifter>
void test_CSE1(T* first, int count, T v1, const std::string label) {
    start_timer();

    for(int i = 0; i < iterations; ++i) {
        T result(0);
        result += first[0] + Shifter::do_shift( v1, first[0], first[1] ) + Shifter::do_shift( v1, first[0], first[1] );
        result -= first[1] + Shifter::do_shift( v1, first[0], first[1] ) + Shifter::do_shift( v1, first[0], first[1] );
        for (int n = 1; n < count; ++n) {
            result += first[n-1] + Shifter::do_shift( v1, first[n-1], first[n] ) + Shifter::do_shift( v1, first[n-1], first[n] );
            result -= first[n]   + Shifter::do_shift( v1, first[n-1], first[n] ) + Shifter::do_shift( v1, first[n-1], first[n] );
        }
        check_shifted_variable_sum_CSE<T, Shifter>(result, v1, label);
    }
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

// inline the work, to test template versus inline CSE (shouldn't be different, but apparently is)
template <typename T, typename Shifter>
void test_CSE1_add_inline(T* first, int count, T v1, const std::string label) {
    start_timer();

    for(int i = 0; i < iterations; ++i) {
        T result(0);
        result += first[0] + (v1 + (first[0] + first[1])) + (v1 + (first[0] + first[1]));
        result -= first[1] + (v1 + (first[0] + first[1])) + (v1 + (first[0] + first[1]));
        for (int n = 1; n < count; ++n) {
            result += first[n-1] + (v1 + (first[n-1] + first[n])) + (v1 + (first[n-1] + first[n]));
            result -= first[n]   + (v1 + (first[n-1] + first[n])) + (v1 + (first[n-1] + first[n]));
        }
        check_shifted_variable_sum_CSE<T, Shifter>(result, v1, label);
    }
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

// inline the work, to test template versus inline CSE (shouldn't be different, but apparently is)
// flip order of some arguments, but remain equivalent expressions
template <typename T, typename Shifter>
void test_CSE1_add_inline_flipped(T* first, int count, T v1, const std::string label) {
    start_timer();

    for(int i = 0; i < iterations; ++i) {
        T result(0);
        result += first[0] + (v1 + (first[0] + first[1])) + (v1 + (first[1] + first[0]));
        result -= first[1] + ((first[1] + first[0]) + v1) + ((first[0] + first[1]) + v1);
        for (int n = 1; n < count; ++n) {
            result += first[n-1] + (v1 + (first[n-1] + first[n])) + (v1 + (first[n] + first[n-1]));
            result -= first[n]   + ((first[n] + first[n-1]) + v1) + ((first[n-1] + first[n]) + v1);
        }
        check_shifted_variable_sum_CSE<T, Shifter>(result, v1, label);
    }
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

template <typename T, typename Shifter>
void test_CSE2_fullopt(T* first, int count, T v1, const std::string label) {
    start_timer();

    // This is about as far as most compilers can optmize the code.
    // So far none realize that all array entries have the same value and result is always zero.
    for(int i = 0; i < iterations; ++i) {
        T result(0);
        T temp2 = (first[0] - first[1]);
        result += temp2;    // should reduce to a shift for integers
        result += temp2;
        for (int n = 1; n < count; ++n) {
            T temp1 = (first[n-1] - first[n]);
            result += temp1;    // should reduce to a shift for integers
            result += temp1;
        }
        check_shifted_variable_sum_CSE<T, Shifter>(result, v1, label);
    }
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

template <typename T, typename Shifter>
void test_CSE2_halfopt(T* first, int count, T v1, const std::string label) {
    start_timer();

    for(int i = 0; i < iterations; ++i) {
        T result(0);
        T temp0 = Shifter::do_shift( v1, first[0], first[1] );
        temp0 += temp0;
        T temp2 = (first[0] + temp0) - (first[1] + temp0);  // algebraic simplification should eliminate temp0 and it's calculation
        result += temp2;    // should reduce to a shift for integers
        result += temp2;
        for (int n = 1; n < count; ++n) {
            T temp = Shifter::do_shift( v1, first[n-1], first[n] );
            temp += temp;
            T temp1 = (first[n-1] + temp) - (first[n] + temp); // algebraic simplification should eliminate temp and it's calculation
            result += temp1;    // should reduce to a shift for integers
            result += temp1;
        }
        check_shifted_variable_sum_CSE<T, Shifter>(result, v1, label);
    }
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

template <typename T, typename Shifter>
void test_CSE2(T* first, int count, T v1, const std::string label) {
    start_timer();

    for(int i = 0; i < iterations; ++i) {
        T result(0);
        result += first[0] + Shifter::do_shift( v1, first[0], first[1] ) + Shifter::do_shift( v1, first[0], first[1] );
        result -= first[1] + Shifter::do_shift( v1, first[0], first[1] ) + Shifter::do_shift( v1, first[0], first[1] );
        result += first[0] + Shifter::do_shift( v1, first[0], first[1] ) + Shifter::do_shift( v1, first[0], first[1] );
        result -= first[1] + Shifter::do_shift( v1, first[0], first[1] ) + Shifter::do_shift( v1, first[0], first[1] );
        for (int n = 1; n < count; ++n) {
            result += first[n-1] + Shifter::do_shift( v1, first[n-1], first[n] ) + Shifter::do_shift( v1, first[n-1], first[n] );
            result -= first[n]   + Shifter::do_shift( v1, first[n-1], first[n] ) + Shifter::do_shift( v1, first[n-1], first[n] );
            result += first[n-1] + Shifter::do_shift( v1, first[n-1], first[n] ) + Shifter::do_shift( v1, first[n-1], first[n] );
            result -= first[n]   + Shifter::do_shift( v1, first[n-1], first[n] ) + Shifter::do_shift( v1, first[n-1], first[n] );
        }
        check_shifted_variable_sum_CSE<T, Shifter>(result, v1, label);
    }
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

// inline the work, to test template versus inline CSE
template <typename T, typename Shifter>
void test_CSE2_add_inline(T* first, int count, T v1, const std::string label) {
    start_timer();

    for(int i = 0; i < iterations; ++i) {
        T result(0);
        result += first[0] + (v1 + (first[0] + first[1])) + (v1 + (first[0] + first[1]));
        result -= first[1] + (v1 + (first[0] + first[1])) + (v1 + (first[0] + first[1]));
        result += first[0] + (v1 + (first[0] + first[1])) + (v1 + (first[0] + first[1]));
        result -= first[1] + (v1 + (first[0] + first[1])) + (v1 + (first[0] + first[1]));
        for (int n = 1; n < count; ++n) {
            result += first[n-1] + (v1 + (first[n-1] + first[n])) + (v1 + (first[n-1] + first[n]));
            result -= first[n]   + (v1 + (first[n-1] + first[n])) + (v1 + (first[n-1] + first[n]));
            result += first[n-1] + (v1 + (first[n-1] + first[n])) + (v1 + (first[n-1] + first[n]));
            result -= first[n]   + (v1 + (first[n-1] + first[n])) + (v1 + (first[n-1] + first[n]));
        }
        check_shifted_variable_sum_CSE<T, Shifter>(result, v1, label);
    }
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

// inline the work, to test template versus inline CSE
// flip order of some arguments, but remain equivalent expressions
template <typename T, typename Shifter>
void test_CSE2_add_inline_flipped( const T* first, const int count, const T v1, const std::string label) {
    start_timer();

    for(int i = 0; i < iterations; ++i) {
        T result(0);
        result += first[0] + (v1 + (first[0] + first[1])) + (v1 + (first[1] + first[0]));
        result -= first[1] + ((first[1] + first[0]) + v1) + ((first[0] + first[1]) + v1);
        result += first[0] + (v1 + (first[0] + first[1])) + (v1 + (first[1] + first[0]));
        result -= first[1] + ((first[1] + first[0]) + v1) + ((first[0] + first[1]) + v1);
        for (int n = 1; n < count; ++n) {
            result += first[n-1] + (v1 + (first[n-1] + first[n])) + (v1 + (first[n] + first[n-1]));
            result -= first[n]   + ((first[n] + first[n-1]) + v1) + ((first[n-1] + first[n]) + v1);
            result += first[n-1] + (v1 + (first[n-1] + first[n])) + (v1 + (first[n] + first[n-1]));
            result -= first[n]   + ((first[n] + first[n-1]) + v1) + ((first[n-1] + first[n]) + v1);
        }
        check_shifted_variable_sum_CSE<T, Shifter>(result, v1, label);
    }
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

template <typename T, typename Shifter>
void test_CSE4_fullopt(T* first, int count, T v1, const std::string label) {
    start_timer();

    // This is about as far as most compilers can optmize the code.
    // So far none realize that all array entries have the same value and result is always zero.
    for(int i = 0; i < iterations; ++i) {
        T result(0);
        T temp2 = (first[0] - first[1]);
        result += temp2;    // should reduce to a shift for integers
        result += temp2;
        result += temp2;
        result += temp2;
        for (int n = 1; n < count; ++n) {
            T temp = Shifter::do_shift( v1, first[n-1], first[n] );
            temp += temp;
            T temp1 = (first[n-1] - first[n]);
            result += temp1;    // should reduce to a shift for integers
            result += temp1;
            result += temp1;
            result += temp1;
        }
        check_shifted_variable_sum_CSE<T, Shifter>(result, v1, label);
    }
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

template <typename T, typename Shifter>
void test_CSE4_halfopt(T* first, int count, T v1, const std::string label) {
    start_timer();

    for(int i = 0; i < iterations; ++i) {
        T result(0);
        T temp0 = Shifter::do_shift( v1, first[0], first[1] );
        temp0 += temp0;
        T temp2 = (first[0] + temp0) - (first[1] + temp0);  // algebraic simplification should eliminate temp0 and it's calculation
        result += temp2;    // should reduce to a shift for integers
        result += temp2;
        result += temp2;
        result += temp2;
        for (int n = 1; n < count; ++n) {
            T temp = Shifter::do_shift( v1, first[n-1], first[n] );
            temp += temp;
            T temp1 = (first[n-1] + temp) - (first[n] + temp); // algebraic simplification should eliminate temp and it's calculation
            result += temp1;    // should reduce to a shift for integers
            result += temp1;
            result += temp1;
            result += temp1;
        }
        check_shifted_variable_sum_CSE<T, Shifter>(result, v1, label);
    }
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

template <typename T, typename Shifter>
void test_CSE4(T* first, int count, T v1, const std::string label) {
    start_timer();

    for(int i = 0; i < iterations; ++i) {
        T result(0);
        result += first[0] + Shifter::do_shift( v1, first[0], first[1] ) + Shifter::do_shift( v1, first[0], first[1] );
        result -= first[1] + Shifter::do_shift( v1, first[0], first[1] ) + Shifter::do_shift( v1, first[0], first[1] );
        result += first[0] + Shifter::do_shift( v1, first[0], first[1] ) + Shifter::do_shift( v1, first[0], first[1] );
        result -= first[1] + Shifter::do_shift( v1, first[0], first[1] ) + Shifter::do_shift( v1, first[0], first[1] );
        result += first[0] + Shifter::do_shift( v1, first[0], first[1] ) + Shifter::do_shift( v1, first[0], first[1] );
        result -= first[1] + Shifter::do_shift( v1, first[0], first[1] ) + Shifter::do_shift( v1, first[0], first[1] );
        result += first[0] + Shifter::do_shift( v1, first[0], first[1] ) + Shifter::do_shift( v1, first[0], first[1] );
        result -= first[1] + Shifter::do_shift( v1, first[0], first[1] ) + Shifter::do_shift( v1, first[0], first[1] );
        for (int n = 1; n < count; ++n) {
            result += first[n-1] + Shifter::do_shift( v1, first[n-1], first[n] ) + Shifter::do_shift( v1, first[n-1], first[n] );
            result -= first[n]   + Shifter::do_shift( v1, first[n-1], first[n] ) + Shifter::do_shift( v1, first[n-1], first[n] );
            result += first[n-1] + Shifter::do_shift( v1, first[n-1], first[n] ) + Shifter::do_shift( v1, first[n-1], first[n] );
            result -= first[n]   + Shifter::do_shift( v1, first[n-1], first[n] ) + Shifter::do_shift( v1, first[n-1], first[n] );
            result += first[n-1] + Shifter::do_shift( v1, first[n-1], first[n] ) + Shifter::do_shift( v1, first[n-1], first[n] );
            result -= first[n]   + Shifter::do_shift( v1, first[n-1], first[n] ) + Shifter::do_shift( v1, first[n-1], first[n] );
            result += first[n-1] + Shifter::do_shift( v1, first[n-1], first[n] ) + Shifter::do_shift( v1, first[n-1], first[n] );
            result -= first[n]   + Shifter::do_shift( v1, first[n-1], first[n] ) + Shifter::do_shift( v1, first[n-1], first[n] );
        }
        check_shifted_variable_sum_CSE<T, Shifter>(result, v1, label);
    }
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

// inline the work, to test template versus inline CSE
template <typename T, typename Shifter>
void test_CSE4_add_inline(T* first, int count, T v1, const std::string label) {
    start_timer();

    for(int i = 0; i < iterations; ++i) {
        T result(0);
        result += first[0] + (v1 + (first[0] + first[1])) + (v1 + (first[0] + first[1]));
        result -= first[1] + (v1 + (first[0] + first[1])) + (v1 + (first[0] + first[1]));
        result += first[0] + (v1 + (first[0] + first[1])) + (v1 + (first[0] + first[1]));
        result -= first[1] + (v1 + (first[0] + first[1])) + (v1 + (first[0] + first[1]));
        result += first[0] + (v1 + (first[0] + first[1])) + (v1 + (first[0] + first[1]));
        result -= first[1] + (v1 + (first[0] + first[1])) + (v1 + (first[0] + first[1]));
        result += first[0] + (v1 + (first[0] + first[1])) + (v1 + (first[0] + first[1]));
        result -= first[1] + (v1 + (first[0] + first[1])) + (v1 + (first[0] + first[1]));
        for (int n = 1; n < count; ++n) {
            result += first[n-1] + (v1 + (first[n-1] + first[n])) + (v1 + (first[n-1] + first[n]));
            result -= first[n]   + (v1 + (first[n-1] + first[n])) + (v1 + (first[n-1] + first[n]));
            result += first[n-1] + (v1 + (first[n-1] + first[n])) + (v1 + (first[n-1] + first[n]));
            result -= first[n]   + (v1 + (first[n-1] + first[n])) + (v1 + (first[n-1] + first[n]));
            result += first[n-1] + (v1 + (first[n-1] + first[n])) + (v1 + (first[n-1] + first[n]));
            result -= first[n]   + (v1 + (first[n-1] + first[n])) + (v1 + (first[n-1] + first[n]));
            result += first[n-1] + (v1 + (first[n-1] + first[n])) + (v1 + (first[n-1] + first[n]));
            result -= first[n]   + (v1 + (first[n-1] + first[n])) + (v1 + (first[n-1] + first[n]));
        }
        check_shifted_variable_sum_CSE<T, Shifter>(result, v1, label);
    }
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

// inline the work, to test template versus inline CSE
// flip order of some arguments, but remain equivalent expressions
template <typename T, typename Shifter>
void test_CSE4_add_inline_flipped(T* first, int count, T v1, const std::string label) {
    start_timer();

    for(int i = 0; i < iterations; ++i) {
        T result(0);
        result += first[0] + (v1 + (first[0] + first[1])) + (v1 + (first[1] + first[0]));
        result -= first[1] + ((first[1] + first[0]) + v1) + ((first[0] + first[1]) + v1);
        result += first[0] + (v1 + (first[0] + first[1])) + (v1 + (first[1] + first[0]));
        result -= first[1] + ((first[1] + first[0]) + v1) + ((first[0] + first[1]) + v1);
        result += first[0] + (v1 + (first[0] + first[1])) + (v1 + (first[1] + first[0]));
        result -= first[1] + ((first[1] + first[0]) + v1) + ((first[0] + first[1]) + v1);
        result += first[0] + (v1 + (first[0] + first[1])) + (v1 + (first[1] + first[0]));
        result -= first[1] + ((first[1] + first[0]) + v1) + ((first[0] + first[1]) + v1);
        for (int n = 1; n < count; ++n) {
            result += first[n-1] + (v1 + (first[n-1] + first[n])) + (v1 + (first[n] + first[n-1]));
            result -= first[n]   + ((first[n] + first[n-1]) + v1) + ((first[n-1] + first[n]) + v1);
            result += first[n-1] + (v1 + (first[n-1] + first[n])) + (v1 + (first[n] + first[n-1]));
            result -= first[n]   + ((first[n] + first[n-1]) + v1) + ((first[n-1] + first[n]) + v1);
            result += first[n-1] + (v1 + (first[n-1] + first[n])) + (v1 + (first[n] + first[n-1]));
            result -= first[n]   + ((first[n] + first[n-1]) + v1) + ((first[n-1] + first[n]) + v1);
            result += first[n-1] + (v1 + (first[n-1] + first[n])) + (v1 + (first[n] + first[n-1]));
            result -= first[n]   + ((first[n] + first[n-1]) + v1) + ((first[n-1] + first[n]) + v1);
        }
        check_shifted_variable_sum_CSE<T, Shifter>(result, v1, label);
    }
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

template <typename T, typename Shifter>
void test_CSE8_fullopt(T* first, int count, T v1, const std::string label) {
    start_timer();

    // This is about as far as most compilers can optmize the code.
    // So far none realize that all array entries have the same value and result is always zero.
    for(int i = 0; i < iterations; ++i) {
        T result(0);
        T temp2 = (first[0] - first[1]);
        result += temp2;    // should reduce to a shift for integers
        result += temp2;
        result += temp2;
        result += temp2;
        result += temp2;
        result += temp2;
        result += temp2;
        result += temp2;
        for (int n = 1; n < count; ++n) {
            T temp1 = (first[n-1] - first[n]);
            result += temp1;    // should reduce to a shift for integers
            result += temp1;
            result += temp1;
            result += temp1;
            result += temp1;
            result += temp1;
            result += temp1;
            result += temp1;
        }
        check_shifted_variable_sum_CSE<T, Shifter>(result, v1, label);
    }
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

template <typename T, typename Shifter>
void test_CSE8_halfopt(T* first, int count, T v1, const std::string label) {
    start_timer();

    for(int i = 0; i < iterations; ++i) {
        T result(0);
        T temp0 = Shifter::do_shift( v1, first[0], first[1] );
        temp0 += temp0;
        T temp2 = (first[0] + temp0) - (first[1] + temp0);  // algebraic simplification should eliminate temp0 and it's calculation
        result += temp2;    // should reduce to a shift for integers
        result += temp2;
        result += temp2;
        result += temp2;
        result += temp2;
        result += temp2;
        result += temp2;
        result += temp2;
        for (int n = 1; n < count; ++n) {
            T temp = Shifter::do_shift( v1, first[n-1], first[n] );
            temp += temp;
            T temp1 = (first[n-1] + temp) - (first[n] + temp); // algebraic simplification should eliminate temp and it's calculation
            result += temp1;    // should reduce to a shift for integers
            result += temp1;
            result += temp1;
            result += temp1;
            result += temp1;
            result += temp1;
            result += temp1;
            result += temp1;
        }
        check_shifted_variable_sum_CSE<T, Shifter>(result, v1, label);
    }
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

template <typename T, typename Shifter>
void test_CSE8(T* first, int count, T v1, const std::string label) {
    start_timer();

    for(int i = 0; i < iterations; ++i) {
        T result(0);
        result += first[0] + Shifter::do_shift( v1, first[0], first[1] ) + Shifter::do_shift( v1, first[0], first[1] );
        result -= first[1] + Shifter::do_shift( v1, first[0], first[1] ) + Shifter::do_shift( v1, first[0], first[1] );
        result += first[0] + Shifter::do_shift( v1, first[0], first[1] ) + Shifter::do_shift( v1, first[0], first[1] );
        result -= first[1] + Shifter::do_shift( v1, first[0], first[1] ) + Shifter::do_shift( v1, first[0], first[1] );
        result += first[0] + Shifter::do_shift( v1, first[0], first[1] ) + Shifter::do_shift( v1, first[0], first[1] );
        result -= first[1] + Shifter::do_shift( v1, first[0], first[1] ) + Shifter::do_shift( v1, first[0], first[1] );
        result += first[0] + Shifter::do_shift( v1, first[0], first[1] ) + Shifter::do_shift( v1, first[0], first[1] );
        result -= first[1] + Shifter::do_shift( v1, first[0], first[1] ) + Shifter::do_shift( v1, first[0], first[1] );
        result += first[0] + Shifter::do_shift( v1, first[0], first[1] ) + Shifter::do_shift( v1, first[0], first[1] );
        result -= first[1] + Shifter::do_shift( v1, first[0], first[1] ) + Shifter::do_shift( v1, first[0], first[1] );
        result += first[0] + Shifter::do_shift( v1, first[0], first[1] ) + Shifter::do_shift( v1, first[0], first[1] );
        result -= first[1] + Shifter::do_shift( v1, first[0], first[1] ) + Shifter::do_shift( v1, first[0], first[1] );
        result += first[0] + Shifter::do_shift( v1, first[0], first[1] ) + Shifter::do_shift( v1, first[0], first[1] );
        result -= first[1] + Shifter::do_shift( v1, first[0], first[1] ) + Shifter::do_shift( v1, first[0], first[1] );
        result += first[0] + Shifter::do_shift( v1, first[0], first[1] ) + Shifter::do_shift( v1, first[0], first[1] );
        result -= first[1] + Shifter::do_shift( v1, first[0], first[1] ) + Shifter::do_shift( v1, first[0], first[1] );
        for (int n = 1; n < count; ++n) {
            result += first[n-1] + Shifter::do_shift( v1, first[n-1], first[n] ) + Shifter::do_shift( v1, first[n-1], first[n] );
            result -= first[n]   + Shifter::do_shift( v1, first[n-1], first[n] ) + Shifter::do_shift( v1, first[n-1], first[n] );
            result += first[n-1] + Shifter::do_shift( v1, first[n-1], first[n] ) + Shifter::do_shift( v1, first[n-1], first[n] );
            result -= first[n]   + Shifter::do_shift( v1, first[n-1], first[n] ) + Shifter::do_shift( v1, first[n-1], first[n] );
            result += first[n-1] + Shifter::do_shift( v1, first[n-1], first[n] ) + Shifter::do_shift( v1, first[n-1], first[n] );
            result -= first[n]   + Shifter::do_shift( v1, first[n-1], first[n] ) + Shifter::do_shift( v1, first[n-1], first[n] );
            result += first[n-1] + Shifter::do_shift( v1, first[n-1], first[n] ) + Shifter::do_shift( v1, first[n-1], first[n] );
            result -= first[n]   + Shifter::do_shift( v1, first[n-1], first[n] ) + Shifter::do_shift( v1, first[n-1], first[n] );
            result += first[n-1] + Shifter::do_shift( v1, first[n-1], first[n] ) + Shifter::do_shift( v1, first[n-1], first[n] );
            result -= first[n]   + Shifter::do_shift( v1, first[n-1], first[n] ) + Shifter::do_shift( v1, first[n-1], first[n] );
            result += first[n-1] + Shifter::do_shift( v1, first[n-1], first[n] ) + Shifter::do_shift( v1, first[n-1], first[n] );
            result -= first[n]   + Shifter::do_shift( v1, first[n-1], first[n] ) + Shifter::do_shift( v1, first[n-1], first[n] );
            result += first[n-1] + Shifter::do_shift( v1, first[n-1], first[n] ) + Shifter::do_shift( v1, first[n-1], first[n] );
            result -= first[n]   + Shifter::do_shift( v1, first[n-1], first[n] ) + Shifter::do_shift( v1, first[n-1], first[n] );
            result += first[n-1] + Shifter::do_shift( v1, first[n-1], first[n] ) + Shifter::do_shift( v1, first[n-1], first[n] );
            result -= first[n]   + Shifter::do_shift( v1, first[n-1], first[n] ) + Shifter::do_shift( v1, first[n-1], first[n] );
        }
        check_shifted_variable_sum_CSE<T, Shifter>(result, v1, label);
    }
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

// inline the work, to test template versus inline CSE
template <typename T, typename Shifter>
void test_CSE8_add_inline(T* first, int count, T v1, const std::string label) {
    start_timer();

    for(int i = 0; i < iterations; ++i) {
        T result(0);
        result += first[0] + (v1 + (first[0] + first[1])) + (v1 + (first[0] + first[1]));
        result -= first[1] + (v1 + (first[0] + first[1])) + (v1 + (first[0] + first[1]));
        result += first[0] + (v1 + (first[0] + first[1])) + (v1 + (first[0] + first[1]));
        result -= first[1] + (v1 + (first[0] + first[1])) + (v1 + (first[0] + first[1]));
        result += first[0] + (v1 + (first[0] + first[1])) + (v1 + (first[0] + first[1]));
        result -= first[1] + (v1 + (first[0] + first[1])) + (v1 + (first[0] + first[1]));
        result += first[0] + (v1 + (first[0] + first[1])) + (v1 + (first[0] + first[1]));
        result -= first[1] + (v1 + (first[0] + first[1])) + (v1 + (first[0] + first[1]));
        result += first[0] + (v1 + (first[0] + first[1])) + (v1 + (first[0] + first[1]));
        result -= first[1] + (v1 + (first[0] + first[1])) + (v1 + (first[0] + first[1]));
        result += first[0] + (v1 + (first[0] + first[1])) + (v1 + (first[0] + first[1]));
        result -= first[1] + (v1 + (first[0] + first[1])) + (v1 + (first[0] + first[1]));
        result += first[0] + (v1 + (first[0] + first[1])) + (v1 + (first[0] + first[1]));
        result -= first[1] + (v1 + (first[0] + first[1])) + (v1 + (first[0] + first[1]));
        result += first[0] + (v1 + (first[0] + first[1])) + (v1 + (first[0] + first[1]));
        result -= first[1] + (v1 + (first[0] + first[1])) + (v1 + (first[0] + first[1]));
        for (int n = 1; n < count; ++n) {
            result += first[n-1] + (v1 + (first[n-1] + first[n])) + (v1 + (first[n-1] + first[n]));
            result -= first[n]   + (v1 + (first[n-1] + first[n])) + (v1 + (first[n-1] + first[n]));
            result += first[n-1] + (v1 + (first[n-1] + first[n])) + (v1 + (first[n-1] + first[n]));
            result -= first[n]   + (v1 + (first[n-1] + first[n])) + (v1 + (first[n-1] + first[n]));
            result += first[n-1] + (v1 + (first[n-1] + first[n])) + (v1 + (first[n-1] + first[n]));
            result -= first[n]   + (v1 + (first[n-1] + first[n])) + (v1 + (first[n-1] + first[n]));
            result += first[n-1] + (v1 + (first[n-1] + first[n])) + (v1 + (first[n-1] + first[n]));
            result -= first[n]   + (v1 + (first[n-1] + first[n])) + (v1 + (first[n-1] + first[n]));
            result += first[n-1] + (v1 + (first[n-1] + first[n])) + (v1 + (first[n-1] + first[n]));
            result -= first[n]   + (v1 + (first[n-1] + first[n])) + (v1 + (first[n-1] + first[n]));
            result += first[n-1] + (v1 + (first[n-1] + first[n])) + (v1 + (first[n-1] + first[n]));
            result -= first[n]   + (v1 + (first[n-1] + first[n])) + (v1 + (first[n-1] + first[n]));
            result += first[n-1] + (v1 + (first[n-1] + first[n])) + (v1 + (first[n-1] + first[n]));
            result -= first[n]   + (v1 + (first[n-1] + first[n])) + (v1 + (first[n-1] + first[n]));
            result += first[n-1] + (v1 + (first[n-1] + first[n])) + (v1 + (first[n-1] + first[n]));
            result -= first[n]   + (v1 + (first[n-1] + first[n])) + (v1 + (first[n-1] + first[n]));
        }
        check_shifted_variable_sum_CSE<T, Shifter>(result, v1, label);
    }
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

// inline the work, to test template versus inline CSE
// flip order of some arguments, but remain equivalent expressions
template <typename T, typename Shifter>
void test_CSE8_add_inline_flipped(T* first, int count, T v1, const std::string label) {
    start_timer();

    for(int i = 0; i < iterations; ++i) {
        T result(0);
        result += first[0] + (v1 + (first[0] + first[1])) + (v1 + (first[1] + first[0]));
        result -= first[1] + ((first[1] + first[0]) + v1) + ((first[0] + first[1]) + v1);
        result += first[0] + (v1 + (first[0] + first[1])) + (v1 + (first[1] + first[0]));
        result -= first[1] + ((first[1] + first[0]) + v1) + ((first[0] + first[1]) + v1);
        result += first[0] + (v1 + (first[0] + first[1])) + (v1 + (first[1] + first[0]));
        result -= first[1] + ((first[1] + first[0]) + v1) + ((first[0] + first[1]) + v1);
        result += first[0] + (v1 + (first[0] + first[1])) + (v1 + (first[1] + first[0]));
        result -= first[1] + ((first[1] + first[0]) + v1) + ((first[0] + first[1]) + v1);
        result += first[0] + (v1 + (first[0] + first[1])) + (v1 + (first[1] + first[0]));
        result -= first[1] + ((first[1] + first[0]) + v1) + ((first[0] + first[1]) + v1);
        result += first[0] + (v1 + (first[0] + first[1])) + (v1 + (first[1] + first[0]));
        result -= first[1] + ((first[1] + first[0]) + v1) + ((first[0] + first[1]) + v1);
        result += first[0] + (v1 + (first[0] + first[1])) + (v1 + (first[1] + first[0]));
        result -= first[1] + ((first[1] + first[0]) + v1) + ((first[0] + first[1]) + v1);
        result += first[0] + (v1 + (first[0] + first[1])) + (v1 + (first[1] + first[0]));
        result -= first[1] + ((first[1] + first[0]) + v1) + ((first[0] + first[1]) + v1);
        for (int n = 1; n < count; ++n) {
            result += first[n-1] + (v1 + (first[n-1] + first[n])) + (v1 + (first[n] + first[n-1]));
            result -= first[n]   + ((first[n] + first[n-1]) + v1) + ((first[n-1] + first[n]) + v1);
            result += first[n-1] + (v1 + (first[n-1] + first[n])) + (v1 + (first[n] + first[n-1]));
            result -= first[n]   + ((first[n] + first[n-1]) + v1) + ((first[n-1] + first[n]) + v1);
            result += first[n-1] + (v1 + (first[n-1] + first[n])) + (v1 + (first[n] + first[n-1]));
            result -= first[n]   + ((first[n] + first[n-1]) + v1) + ((first[n-1] + first[n]) + v1);
            result += first[n-1] + (v1 + (first[n-1] + first[n])) + (v1 + (first[n] + first[n-1]));
            result -= first[n]   + ((first[n] + first[n-1]) + v1) + ((first[n-1] + first[n]) + v1);
            result += first[n-1] + (v1 + (first[n-1] + first[n])) + (v1 + (first[n] + first[n-1]));
            result -= first[n]   + ((first[n] + first[n-1]) + v1) + ((first[n-1] + first[n]) + v1);
            result += first[n-1] + (v1 + (first[n-1] + first[n])) + (v1 + (first[n] + first[n-1]));
            result -= first[n]   + (v1 + (first[n] + first[n-1])) + (v1 + (first[n-1] + first[n]));
            result += first[n-1] + (v1 + (first[n-1] + first[n])) + (v1 + (first[n] + first[n-1]));
            result -= first[n]   + ((first[n] + first[n-1]) + v1) + ((first[n-1] + first[n]) + v1);
            result += first[n-1] + (v1 + (first[n-1] + first[n])) + (v1 + (first[n] + first[n-1]));
            result -= first[n]   + ((first[n] + first[n-1]) + v1) + ((first[n-1] + first[n]) + v1);
        }
        check_shifted_variable_sum_CSE<T, Shifter>(result, v1, label);
    }
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}



/******************************************************************************/

// this is the heart of our loop unrolling - a class that unrolls itself to generate the inner loop code
// at least as long as we keep F < 50 (or some compilers won't compile it)
template< int F, typename T, class Shifter >
struct loop_inner_body {
    inline static void do_work(T &result, const T *first, const T v1, const size_t n) {
        static_assert(F > 0, "bad unroll by 1");
        result += first[n-1] + Shifter::do_shift(v1,first[n],first[n-1]) + Shifter::do_shift(v1,first[n],first[n-1]);
        result -= first[n]   + Shifter::do_shift(v1,first[n],first[n-1]) + Shifter::do_shift(v1,first[n],first[n-1]);
        loop_inner_body<(F-1),T,Shifter>::do_work(result,first,v1,n);
    }
};

template< typename T, typename Shifter >
struct loop_inner_body<0,T,Shifter> {
    inline static void do_work(T &, const T *, const T, const size_t) {
    }
};

/******************************************************************************/

#if WORKS_BUT_COMPILERS_FAIL_TO_OPTIMIZE
// saved for later testing, when basic optimizations are working

template< int F, typename T, class Shifter >
struct loop_inner_body2 {
    inline static void do_work(T &result, const T *first, const T v1, size_t n) {
        static_assert(F > 0, "bad unroll by 2");
        result += first[n-1] + Shifter::do_shift(v1,first[n],first[n-1]) + Shifter::do_shift(v1,first[n],first[n-1]);
        result -= first[n]   + Shifter::do_shift(v1,first[n],first[n-1]) + Shifter::do_shift(v1,first[n],first[n-1]);
        const int temp = (F-1)/2;
        loop_inner_body2<temp,T,Shifter>::do_work(result,first,v1,n);
        loop_inner_body2<((F-1)-temp),T,Shifter>::do_work(result,first,v1,n);
    }
};

template< typename T, typename Shifter >
struct loop_inner_body2<0,T,Shifter> {
    inline static void do_work(T &, const T *, const T, const size_t) {
    }
};

#endif

/******************************************************************************/

#if WORKS_BUT_COMPILERS_FAIL_TO_OPTIMIZE
// saved for later testing, when basic optimizations are working

template< int F, typename T, class Shifter >
struct loop_inner_body4 {
    inline static void do_work(T &result, const T *first, const T v1, const size_t n) {
        static_assert(F > 0, "bad unroll by 4");
        result += first[n-1] + Shifter::do_shift(v1,first[n],first[n-1]) + Shifter::do_shift(v1,first[n],first[n-1]);
        result -= first[n]   + Shifter::do_shift(v1,first[n],first[n-1]) + Shifter::do_shift(v1,first[n],first[n-1]);
        const int temp = (F-1)/4;
        loop_inner_body4<temp,T,Shifter>::do_work(result,first,v1,n);
        loop_inner_body4<temp,T,Shifter>::do_work(result,first,v1,n);
        loop_inner_body4<temp,T,Shifter>::do_work(result,first,v1,n);
        loop_inner_body4<((F-1)-(3*temp)),T,Shifter>::do_work(result,first,v1,n);
    }
};

template< typename T, typename Shifter >
struct loop_inner_body4<0,T,Shifter> {
    inline static void do_work(T &, const T *, const T, const size_t) {
    }
};

#endif

/******************************************************************************/

template <typename T, typename Shifter, int F>
void test_CSEN_fullopt(T* first, int count, T v1, const std::string label) {
    start_timer();

    // This is about as far as most compilers can optmize the code.
    // So far none realize that all array entries have the same value and result is always zero.
    for(int i = 0; i < iterations; ++i) {
        T result(0);
        T temp2 = (first[0] - first[1]) * F;
        result += temp2;
        for (int n = 1; n < count; ++n) {
            T temp1 = (first[n-1] - first[n]) * F;
            result += temp1;
        }
        check_shifted_variable_sum_CSE<T, Shifter>(result, v1, label);
    }
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

template <typename T, typename Shifter, int F>
void test_CSEN_halfopt(T* first, int count, T v1, const std::string label) {
    start_timer();

    for(int i = 0; i < iterations; ++i) {
        T result(0);
        T temp0 = Shifter::do_shift( v1, first[0], first[1] );
        temp0 += temp0;
        T temp2 = (first[0] + temp0) - (first[1] + temp0);  // algebraic simplification should eliminate temp0 and it's calculation
        temp2 *= F;
        result += temp2;
        for (int n = 1; n < count; ++n) {
            T temp = Shifter::do_shift( v1, first[n-1], first[n] );
            temp += temp;
            T temp1 = (first[n-1] + temp) - (first[n] + temp); // algebraic simplification should eliminate temp and it's calculation
            temp1 *= F;
            result += temp1;
        }
        check_shifted_variable_sum_CSE<T, Shifter>(result, v1, label);
    }
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

template <typename T, typename Shifter, int F>
void test_CSEN(T* first, int count, T v1, const std::string label) {
    start_timer();

    for(int i = 0; i < iterations; ++i) {
        T result(0);
        loop_inner_body<F,T,Shifter>::do_work(result,first,v1,1);
        for (int n = 1; n < count; ++n) {
            loop_inner_body<F,T,Shifter>::do_work(result,first,v1,n);
        }
        check_shifted_variable_sum_CSE<T, Shifter>(result, v1, label);
    }
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/
/******************************************************************************/

template <typename T>
    struct custom_cse_mix {
      static T do_shift(T v1, T v2, T v3) { return (v1 * (v2 - v3) + (v2 / v1) ); }
    };

/******************************************************************************/

template <typename T>
    struct custom_cse_add {
      static T do_shift(T v1, T v2, T v3) { return (v1 + (v2 + v3) ); }
    };

/******************************************************************************/

template <typename T>
    struct custom_cse_sub {
      static T do_shift(T v1, T v2, T v3) { return (v1 + (v2 - v3) ); }
    };

/******************************************************************************/

template <typename T>
    struct custom_cse_mul {
      static T do_shift(T v1, T v2, T v3) { return (v1 + (v2 * v3) ); }
    };

/******************************************************************************/

template <typename T>
    struct custom_cse_div {
      static T do_shift(T v1, T v2, T v3) { return (v1 + (v2 / v3) ); }
    };

/******************************************************************************/
/******************************************************************************/

template< typename T, int F >
void TestUnrolledType(T* data, T var1, std::string label)
{
    test_CSEN_fullopt< T, custom_cse_add<T>, F > (data, SIZE, var1, label + " add opt");
    test_CSEN_halfopt< T, custom_cse_add<T>, F > (data, SIZE, var1, label + " add half opt");
    test_CSEN< T, custom_cse_add<T>, F > (data, SIZE, var1, label + " add");
    
    test_CSEN_halfopt< T, custom_cse_sub<T>, F > (data, SIZE, var1, label + " subtract half opt");
    test_CSEN< T, custom_cse_sub<T>, F > (data, SIZE, var1, label + " subtract");
    
    test_CSEN_halfopt< T, custom_cse_mul<T>, F > (data, SIZE, var1, label + " multiply half opt");
    test_CSEN< T, custom_cse_mul<T>, F > (data, SIZE, var1, label + " multiply");
    
    test_CSEN_halfopt< T, custom_cse_div<T>, F > (data, SIZE, var1, label + " divide half opt");
    test_CSEN< T, custom_cse_div<T>, F > (data, SIZE, var1, label + " divide");
    
    test_CSEN_halfopt< T, custom_cse_mix<T>, F > (data, SIZE, var1, label + " mix half opt");
    test_CSEN< T, custom_cse_mix<T>, F > (data, SIZE, var1, label + " mix");
    
    summarize(label.c_str(), SIZE, iterations, kDontShowGMeans, kDontShowPenalty );
}

/******************************************************************************/

template< typename T >
void TestOneType(double temp) {

    T data[SIZE];
    
    ::fill(data, data+SIZE, T(init_value));
    
    std::string myTypeName( getTypeName<T>() );
    
    gLabels.clear();
    
    T var1 = T(temp + 1);


    test_CSE1_fullopt< T, custom_cse_add<T> > (data, SIZE, var1, myTypeName + " CSE add opt");
    test_CSE1_halfopt< T, custom_cse_add<T> > (data, SIZE, var1, myTypeName + " CSE add half opt");
    test_CSE1_add_inline< T, custom_cse_add<T> > (data, SIZE, var1, myTypeName + " CSE add inline");
    test_CSE1_add_inline_flipped< T, custom_cse_add<T> > (data, SIZE, var1, myTypeName + " CSE add inline flipped");
    test_CSE1< T, custom_cse_add<T> > (data, SIZE, var1, myTypeName + " CSE add");
    
    test_CSE1_halfopt< T, custom_cse_sub<T> > (data, SIZE, var1, myTypeName + " CSE subtract half opt");
    test_CSE1< T, custom_cse_sub<T> > (data, SIZE, var1, myTypeName + " CSE subtract");
    
    test_CSE1_halfopt< T, custom_cse_mul<T> > (data, SIZE, var1, myTypeName + " CSE multiply half opt");
    test_CSE1< T, custom_cse_mul<T> > (data, SIZE, var1, myTypeName + " CSE multiply");
    
    test_CSE1_halfopt< T, custom_cse_div<T> > (data, SIZE, var1, myTypeName + " CSE divide half opt");
    test_CSE1< T, custom_cse_div<T> > (data, SIZE, var1, myTypeName + " CSE divide");
    
    test_CSE1_halfopt< T, custom_cse_mix<T> > (data, SIZE, var1, myTypeName + " CSE mix half opt");
    test_CSE1< T, custom_cse_mix<T> > (data, SIZE, var1, myTypeName + " CSE mix");

    std::string temp1( myTypeName + " CSE" );
    summarize(temp1.c_str(), SIZE, iterations, kDontShowGMeans, kDontShowPenalty );
    
    
    
    test_CSE2_fullopt< T, custom_cse_add<T> > (data, SIZE, var1, myTypeName + " CSE2X add opt");
    test_CSE2_halfopt< T, custom_cse_add<T> > (data, SIZE, var1, myTypeName + " CSE2X add half opt");
    test_CSE2_add_inline< T, custom_cse_add<T> > (data, SIZE, var1, myTypeName + " CSE2X add inline");
    test_CSE2_add_inline_flipped< T, custom_cse_add<T> > (data, SIZE, var1, myTypeName + " CSE2X add inline flipped");
    test_CSE2< T, custom_cse_add<T> > (data, SIZE, var1, myTypeName + " CSE2X add");
    
    test_CSE2_halfopt< T, custom_cse_sub<T> > (data, SIZE, var1, myTypeName + " CSE2X subtract half opt");
    test_CSE2< T, custom_cse_sub<T> > (data, SIZE, var1, myTypeName + " CSE2X subtract");
    
    test_CSE2_halfopt< T, custom_cse_mul<T> > (data, SIZE, var1, myTypeName + " CSE2X multiply half opt");
    test_CSE2< T, custom_cse_mul<T> > (data, SIZE, var1, myTypeName + " CSE2X multiply");
    
    test_CSE2_halfopt< T, custom_cse_div<T> > (data, SIZE, var1, myTypeName + " CSE2X divide half opt");
    test_CSE2< T, custom_cse_div<T> > (data, SIZE, var1, myTypeName + " CSE2X divide");
    
    test_CSE2_halfopt< T, custom_cse_mix<T> > (data, SIZE, var1, myTypeName + " CSE2X mix half opt");
    test_CSE2< T, custom_cse_mix<T> > (data, SIZE, var1, myTypeName + " CSE2X mix");
    
    std::string temp2( myTypeName + " CSE2X" );
    summarize(temp2.c_str(), SIZE, iterations, kDontShowGMeans, kDontShowPenalty );
    
    
    
    test_CSE4_fullopt< T, custom_cse_add<T> > (data, SIZE, var1, myTypeName + " CSE4X add opt");
    test_CSE4_halfopt< T, custom_cse_add<T> > (data, SIZE, var1, myTypeName + " CSE4X add half opt");
    test_CSE4_add_inline< T, custom_cse_add<T> > (data, SIZE, var1, myTypeName + " CSE4X add inline");
    test_CSE4_add_inline_flipped< T, custom_cse_add<T> > (data, SIZE, var1, myTypeName + " CSE4X add inline flipped");
    test_CSE4< T, custom_cse_add<T> > (data, SIZE, var1, myTypeName + " CSE4X add");
    
    test_CSE4_halfopt< T, custom_cse_sub<T> > (data, SIZE, var1, myTypeName + " CSE4X subtract half opt");
    test_CSE4< T, custom_cse_sub<T> > (data, SIZE, var1, myTypeName + " CSE4X subtract");
    
    test_CSE4_halfopt< T, custom_cse_mul<T> > (data, SIZE, var1, myTypeName + " CSE4X multiply half opt");
    test_CSE4< T, custom_cse_mul<T> > (data, SIZE, var1, myTypeName + " CSE4X multiply");
    
    test_CSE4_halfopt< T, custom_cse_div<T> > (data, SIZE, var1, myTypeName + " CSE4X divide half opt");
    test_CSE4< T, custom_cse_div<T> > (data, SIZE, var1, myTypeName + " CSE4X divide");
    
    test_CSE4_halfopt< T, custom_cse_mix<T> > (data, SIZE, var1, myTypeName + " CSE4X mix half opt");
    test_CSE4< T, custom_cse_mix<T> > (data, SIZE, var1, myTypeName + " CSE4X mix");
    
    std::string temp3( myTypeName + " CSE4X" );
    summarize(temp3.c_str(), SIZE, iterations, kDontShowGMeans, kDontShowPenalty );
    
    
    
    test_CSE8_fullopt< T, custom_cse_add<T> > (data, SIZE, var1, myTypeName + " CSE8X add opt");
    test_CSE8_halfopt< T, custom_cse_add<T> > (data, SIZE, var1, myTypeName + " CSE8X add half opt");
    test_CSE8_add_inline< T, custom_cse_add<T> > (data, SIZE, var1, myTypeName + " CSE8X add inline");
    test_CSE8_add_inline_flipped< T, custom_cse_add<T> > (data, SIZE, var1, myTypeName + " CSE8X add inline flipped");
    test_CSE8< T, custom_cse_add<T> > (data, SIZE, var1, myTypeName + " CSE8X add");
    
    test_CSE8_halfopt< T, custom_cse_sub<T> > (data, SIZE, var1, myTypeName + " CSE8X subtract half opt");
    test_CSE8< T, custom_cse_sub<T> > (data, SIZE, var1, myTypeName + " CSE8X subtract");
    
    test_CSE8_halfopt< T, custom_cse_mul<T> > (data, SIZE, var1, myTypeName + " CSE8X multiply half opt");
    test_CSE8< T, custom_cse_mul<T> > (data, SIZE, var1, myTypeName + " CSE8X multiply");
    
    test_CSE8_halfopt< T, custom_cse_div<T> > (data, SIZE, var1, myTypeName + " CSE8X divide half opt");
    test_CSE8< T, custom_cse_div<T> > (data, SIZE, var1, myTypeName + " CSE8X divide");
    
    test_CSE8_halfopt< T, custom_cse_mix<T> > (data, SIZE, var1, myTypeName + " CSE8X mix half opt");
    test_CSE8< T, custom_cse_mix<T> > (data, SIZE, var1, myTypeName + " CSE8X mix");
    
    std::string temp4( myTypeName + " CSE8X" );
    summarize(temp4.c_str(), SIZE, iterations, kDontShowGMeans, kDontShowPenalty );



    TestUnrolledType<T,4>(data, var1, myTypeName + " CSE4X_unroll");        // int8 283 sec
    TestUnrolledType<T,8>(data, var1, myTypeName + " CSE8X_unroll");        // int8 281 sec
        // unroll2,4: mix fails CSE takes 1175 seconds!

#if WORKS_BUT_COMPILERS_FAIL_TO_OPTIMIZE
// TODO - something is going wrong with the more highly unrolled versions.
// looks like it just stops optimizing after unroll factor 11.
    TestUnrolledType<T,16>(data, var1, myTypeName + " CSE16X_unroll");      // EXTEMELY slow     int8 6617.45 sec! ~= 2 hours!
        // unroll1,2,4: takes 6511 seconds, mix and div fail CSE, others 10x worse for 2x increase in code
    TestUnrolledType<T,32>(data, var1, myTypeName + " CSE32X_unroll");      // EXTEMELY slow     int8 13540.29 sec! == 3.75 hours!
    TestUnrolledType<T,64>(data, var1, myTypeName + " CSE64X_unroll");      // EXTEMELY slow     I ain't gonna wait for it to finish.
#endif

}

/******************************************************************************/


int main(int argc, char** argv) {
    double temp = 1.0;
    
    // output command for documentation:
    int i;
    for (i = 0; i < argc; ++i)
        printf("%s ", argv[i] );
    printf("\n");

    if (argc > 1) iterations = atoi(argv[1]);
    if (argc > 2) init_value = (double) atof(argv[2]);
    if (argc > 3) temp = (double)atof(argv[3]);


    // Different types are showing different issues (mostly with vectorization, some with CSE and algebraic simplification).
    // But a few types show most of the patterns:
    //     so far int8 and int16 show the same problems, int32 and int64 are similar, float and double are similar.
    // Because compilers are so bad at these optimizations, these tests are running slowly.
    // Once compilers are doing better at these tests, we can turn on the remaining types.
    
    
    TestOneType<int8_t>(temp);
    TestOneType<uint8_t>(temp);
    //TestOneType<int16_t>(temp);
    //TestOneType<uint16_t>(temp);
    TestOneType<int32_t>(temp);
    TestOneType<uint32_t>(temp);
    
    // remaining tests are slower
    iterations /= 8;
    
    //TestOneType<int64_t>(temp);
    //TestOneType<uint64_t>(temp);
    TestOneType<float>(temp);
    //TestOneType<double>(temp);
    //TestOneType<long double>(temp);

    
    return 0;
}

// the end
/******************************************************************************/
/******************************************************************************/
