/*
    Copyright 2007-2008 Adobe Systems Incorporated
    Copyright 2018 Chris Cox
    Distributed under the MIT License (see accompanying file LICENSE_1_0_0.txt
    or a copy at http://stlab.adobe.com/licenses.html )


Goal:  Test compiler optimizations related to removing unnecessary loops.


Assumptions:
    1) The compiler will normalize loops.
        Additional optimizations should not depend on syntax of loop.
        See also: loop_normalize.cpp

    2) The compiler will remove empty loops.
        aka: dead loop removal

    3) The compiler will remove loops whose contents do not contribute to output / results.
        aka: dead loop removal after removing dead code from loop body
    
    4) The compiler will remove constant length loops when the result can be calculated directly.

    5) The compiler will remove variable length loops when the result can be calculated directly.



NOTE - loops that cannot be entered are covered in dead_code_elimination.cpp

NOTE - I found nested pointless loops worse than this in a physics simulation package.
        Names have been removed to protect the innocent grad students maintaining said package.

*/

#include "benchmark_stdint.hpp"
#include <stddef.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <math.h>
#include "benchmark_results.h"
#include "benchmark_timer.h"

/******************************************************************************/

// this constant may need to be adjusted to give reasonable minimum times
// For best results, times should be about 1.0 seconds for the minimum test run
// In this file, correct results take zero time, so adjust for the bad results.
int iterations = 40000;

#define SIZE    100000

// initial value for filling our arrays, may be changed from the command line
double init_value = 3.0;

// global count for loop multiplication
// value must be less than 8th root of (2^31)-1 or about 14
int count = 6;

int count_array[ 8 ];

/******************************************************************************/

#include "benchmark_shared_tests.h"

/******************************************************************************/

template <typename T>
inline void check_sum(T result, int length) {
  T temp = (length*length)*(length*length)*(length*length)*(length*length);
  if (!tolerance_equal<T>(result,temp)) printf("test %i failed\n", current_test);
}

/******************************************************************************/

template <typename T>
inline void check_sum(T result, int* lengths) {
  T temp = (lengths[0]*lengths[1])*(lengths[2]*lengths[3])*(lengths[4]*lengths[5])*(lengths[6]*lengths[7]);
  if (!tolerance_equal<T>(result,temp)) printf("test %i failed\n", current_test);
}

/******************************************************************************/

template <typename T>
inline void check_sum2(T result) {
  T temp = (T)255 * (T)SIZE;
  if (!tolerance_equal<T>(result,temp)) printf("test %i failed\n", current_test);
}

/******************************************************************************/
/******************************************************************************/

template <typename T >
void test_loop_opt(int length, const char *label) {
    int i;

    start_timer();
  
    for(i = 0; i < iterations; ++i) {
        T result = 0;

        result = (length*length)*(length*length)*(length*length)*(length*length);

        check_sum<T>(result, length);
    }
  
    record_result( timer(), label );
}

/******************************************************************************/

template <typename T >
void test_for_loop_single(int length, const char *label) {
    int i;

    start_timer();
  
    for(i = 0; i < iterations; ++i) {
        T result = 0;
        int x, y, z, w, j, k, i, m;

        for (x = 0; x < length; ++x)
        for (y = 0; y < length; ++y)
        for (z = 0; z < length; ++z)
        for (w = 0; w < length; ++w)
        for (j = 0; j < length; ++j)
        for (k = 0; k < length; ++k)
        for (i = 0; i < length; ++i)
        for (m = 0; m < length; ++m)
            ++result;

        check_sum<T>(result, length);
    }
  
    record_result( timer(), label );
}

/******************************************************************************/

template <typename T >
void test_for_loop_single2(int length, const char *label) {
    int i;

    start_timer();
  
    for(i = 0; i < iterations; ++i) {
        T result = 0;
        int x, y, z, w, j, k, i, m;

        for (x = length; x > 0; --x)
        for (y = length; y > 0; --y)
        for (z = length; z > 0; --z)
        for (w = length; w > 0; --w)
        for (j = length; j > 0; --j)
        for (k = length; k > 0; --k)
        for (i = length; i > 0; --i)
        for (m = length; m > 0; --m)
            ++result;

        check_sum<T>(result, length);
    }
  
    record_result( timer(), label );
}

/******************************************************************************/

template <typename T >
void test_for_loop_single3(int length, const char *label) {
    int i;

    start_timer();
  
    for(i = 0; i < iterations; ++i) {
        T result = 0;
        int x, y, z, w, j, k, i, m;

        for (x = 0; x < length; ++x)
        for (y = length; y > 0; --y)
        for (z = 0; z < length; ++z)
        for (w = length; w > 0; --w)
        for (j = 0; j < length; ++j)
        for (k = length; k > 0; --k)
        for (i = 0; i < length; ++i)
        for (m = length; m > 0; --m)
            ++result;

        check_sum<T>(result, length);
    }
  
    record_result( timer(), label );
}

/******************************************************************************/

template <typename T >
void test_while_loop_single(int length, const char *label) {
    int i;

    start_timer();
  
    for(i = 0; i < iterations; ++i) {
        T result = 0;
        int x, y, z, w, j, k, i, m;

        x = 0;
        while (x < length) {
        y = 0;
        while (y < length) {
        z = 0;
        while (z < length) {
        w = 0;
        while (w < length) {
        j = 0;
        while (j < length) {
        k = 0;
        while (k < length) {
        i = 0;
        while (i < length) {
        m = 0;
        while (m < length) {
            ++result;
        ++m;
        }
        ++i;
        }
        ++k;
        }
        ++j;
        }
        ++w;
        }
        ++z;
        }
        ++y;
        }
        ++x;
        }

        check_sum<T>(result, length);
    }
  
    record_result( timer(), label );
}

/******************************************************************************/

template <typename T >
void test_do_loop_single(int length, const char *label) {
    int i;

    start_timer();
  
    for(i = 0; i < iterations; ++i) {
        T result = 0;
        int x, y, z, w, j, k, i, m;

        x = 0;
        if (length > 0)
        do {
        y = 0;
        do {
        z = 0;
        do {
        w = 0;
        do {
        j = 0;
        do {
        k = 0;
        do {
        i = 0;
        do {
        m = 0;
        do {
            ++result;
        ++m;
        } while (m < length);
        ++i;
        } while (i < length);
        ++k;
        } while (k < length);
        ++j;
        } while (j < length);
        ++w;
        } while (w < length);
        ++z;
        } while (z < length);
        ++y;
        } while (y < length);
        ++x;
        } while (x < length);

        check_sum<T>(result, length);
    }
  
    record_result( timer(), label );
}

/******************************************************************************/

template <typename T >
void test_goto_loop_single(int length, const char *label) {
    int i;

    start_timer();
  
    for(i = 0; i < iterations; ++i) {
        T result = 0;
        int x, y, z, w, j, k, i, m;

        x = 0;
        if (length > 0)
        {
loop_start_x:
        y = 0;
        {
loop_start_y:
        z = 0;
        {
loop_start_z:
        w = 0;
        {
loop_start_w:
        j = 0;
        {
loop_start_j:
        k = 0;
        {
loop_start_k:
        i = 0;
        {
loop_start_i:
        m = 0;
        {
loop_start_m:
            ++result;
        
        ++m;

        if (m < length)
            goto loop_start_m;
        }
        ++i;
        if (i < length)
            goto loop_start_i;
        }
        ++k;
        if (k < length)
            goto loop_start_k;
        }
        ++j;
        if (j < length)
            goto loop_start_j;
        }
        ++w;
        if (w < length)
            goto loop_start_w;
        }
        ++z;
        if (z < length)
            goto loop_start_z;
        }
        ++y;
        if (y < length)
            goto loop_start_y;
        }
        ++x;
        if (x < length)
            goto loop_start_x;
        }

        check_sum<T>(result, length);
    }
  
    record_result( timer(), label );
}

/******************************************************************************/

// scalar replacement of arrays would help here
template <typename T >
void test_for_loop_multiple(int* lengths, const char *label) {
    int i;

    start_timer();
  
    for(i = 0; i < iterations; ++i) {
        T result = 0;
        int x, y, z, w, j, k, i, m;

        for (x = 0; x < lengths[0]; ++x)
        for (y = 0; y < lengths[1]; ++y)
        for (z = 0; z < lengths[2]; ++z)
        for (w = 0; w < lengths[3]; ++w)
        for (j = 0; j < lengths[4]; ++j)
        for (k = 0; k < lengths[5]; ++k)
        for (i = 0; i < lengths[6]; ++i)
        for (m = 0; m < lengths[7]; ++m)
            ++result;

        check_sum<T>(result, lengths);
    }
  
    record_result( timer(), label );
}

/******************************************************************************/

// scalar replacement of arrays would help here
template <typename T >
void test_for_loop_multiple2(int* lengths, const char *label) {
    int i;

    start_timer();
  
    for(i = 0; i < iterations; ++i) {
        T result = 0;
        int x, y, z, w, j, k, i, m;

        for (x = lengths[0]; x > 0; --x)
        for (y = lengths[1]; y > 0; --y)
        for (z = lengths[2]; z > 0; --z)
        for (w = lengths[3]; w > 0; --w)
        for (j = lengths[4]; j > 0; --j)
        for (k = lengths[5]; k > 0; --k)
        for (i = lengths[6]; i > 0; --i)
        for (m = lengths[7]; m > 0; --m)
            ++result;

        check_sum<T>(result, lengths);
    }
  
    record_result( timer(), label );
}

/******************************************************************************/

// scalar replacement of arrays would help here
template <typename T >
void test_for_loop_multiple3(int* lengths, const char *label) {
    int i;

    start_timer();
  
    for(i = 0; i < iterations; ++i) {
        T result = 0;
        int x, y, z, w, j, k, i, m;

        for (x = 0; x < lengths[0]; ++x)
        for (y = lengths[1]; y > 0; --y)
        for (z = 0; z < lengths[2]; ++z)
        for (w = lengths[3]; w > 0; --w)
        for (j = 0; j < lengths[4]; ++j)
        for (k = lengths[5]; k > 0; --k)
        for (i = 0; i < lengths[6]; ++i)
        for (m = lengths[7]; m > 0; --m)
            ++result;

        check_sum<T>(result, lengths);
    }
  
    record_result( timer(), label );
}

/******************************************************************************/

template <typename T >
void test_while_loop_multiple(int* lengths, const char *label) {
    int i;

    start_timer();
  
    for(i = 0; i < iterations; ++i) {
        T result = 0;
        int x, y, z, w, j, k, i, m;

        x = 0;
        while (x < lengths[0]) {
        y = 0;
        while (y < lengths[1]) {
        z = 0;
        while (z < lengths[2]) {
        w = 0;
        while (w < lengths[3]) {
        j = 0;
        while (j < lengths[4]) {
        k = 0;
        while (k < lengths[5]) {
        i = 0;
        while (i < lengths[6]) {
        m = 0;
        while (m < lengths[7]) {
            ++result;
        ++m;
        }
        ++i;
        }
        ++k;
        }
        ++j;
        }
        ++w;
        }
        ++z;
        }
        ++y;
        }
        ++x;
        }

        check_sum<T>(result, lengths);
    }
  
    record_result( timer(), label );
}

/******************************************************************************/

template <typename T >
void test_do_loop_multiple(int* lengths, const char *label) {
    int i;

    start_timer();
  
    for(i = 0; i < iterations; ++i) {
        T result = 0;
        int x, y, z, w, j, k, i, m;

        x = 0;
        if (lengths[0] > 0)
        do {
        y = 0;
        if (lengths[1] > 0)
        do {
        z = 0;
        if (lengths[2] > 0)
        do {
        w = 0;
        if (lengths[3] > 0)
        do {
        j = 0;
        if (lengths[4] > 0)
        do {
        k = 0;
        if (lengths[5] > 0)
        do {
        i = 0;
        if (lengths[6] > 0)
        do {
        m = 0;
        if (lengths[7] > 0)
        do {
            ++result;
        ++m;
        } while (m < lengths[7]);
        ++i;
        } while (i < lengths[6]);
        ++k;
        } while (k < lengths[5]);
        ++j;
        } while (j < lengths[4]);
        ++w;
        } while (w < lengths[3]);
        ++z;
        } while (z < lengths[2]);
        ++y;
        } while (y < lengths[1]);
        ++x;
        } while (x < lengths[0]);

        check_sum<T>(result, lengths);
    }
  
    record_result( timer(), label );
}

/******************************************************************************/

template <typename T >
void test_goto_loop_multiple(int* lengths, const char *label) {
    int i;

    start_timer();
  
    for(i = 0; i < iterations; ++i) {
        T result = 0;
        int x, y, z, w, j, k, i, m;

        x = 0;
        if (lengths[0] > 0)
        {
loop_start_x:
        y = 0;
        if (lengths[1] > 0)
        {
loop_start_y:
        z = 0;
        if (lengths[2] > 0)
        {
loop_start_z:
        w = 0;
        if (lengths[3] > 0)
        {
loop_start_w:
        j = 0;
        if (lengths[4] > 0)
        {
loop_start_j:
        k = 0;
        if (lengths[5] > 0)
        {
loop_start_k:
        i = 0;
        if (lengths[6] > 0)
        {
loop_start_i:
        m = 0;
        if (lengths[7] > 0)
        {
loop_start_m:
            ++result;
        
        ++m;

        if (m < lengths[7])
            goto loop_start_m;
        }
        ++i;
        if (i < lengths[6])
            goto loop_start_i;
        }
        ++k;
        if (k < lengths[5])
            goto loop_start_k;
        }
        ++j;
        if (j < lengths[4])
            goto loop_start_j;
        }
        ++w;
        if (w < lengths[3])
            goto loop_start_w;
        }
        ++z;
        if (z < lengths[2])
            goto loop_start_z;
        }
        ++y;
        if (y < lengths[1])
            goto loop_start_y;
        }
        ++x;
        if (x < lengths[0])
            goto loop_start_x;
        }

        check_sum<T>(result, lengths);
    }

    record_result( timer(), label );
}

/******************************************************************************/
/******************************************************************************/

template <typename T >
void test_loop_const_opt(const char *label) {
    int i;

    start_timer();
  
    for(i = 0; i < iterations; ++i) {
        T result = 0;
        
        result = (T)SIZE * (T)255;

        check_sum2<T>(result);
    }
  
    record_result( timer(), label );
}

/******************************************************************************/

template <typename T >
void test_for_loop_const(const char *label) {
    int i;

    start_timer();
  
    for(i = 0; i < iterations; ++i) {
        T result = 0;
        int x, i;
        
        for (x = 0; x < SIZE; ++x) {
            T temp = 0;
            for (i = 0; i < 8; ++i) {
                temp += T(1L << i);
            }
            result += temp;
        }

        check_sum2<T>(result);
    }
  
    record_result( timer(), label );
}

/******************************************************************************/

template <typename T >
void test_for_loop_const2(const char *label) {
    int i;

    start_timer();
  
    for(i = 0; i < iterations; ++i) {
        T result = 0;
        int x, i;
        
        for (x = SIZE; x > 0; --x) {
            T temp = 0;
            for (i = 0; i < 8; ++i) {
                temp += T(1L << i);
            }
            result += temp;
        }

        check_sum2<T>(result);
    }
  
    record_result( timer(), label );
}

/******************************************************************************/

template <typename T >
void test_for_loop_const3(const char *label) {
    int i;

    start_timer();
  
    for(i = 0; i < iterations; ++i) {
        T result = 0;
        int x, i;
        
        for (x = 0; x < SIZE; ++x) {
            T temp = 0;
            for (i = 0; i < 8; ++i) {
                temp += T(0x80L >> i);
            }
            result += temp;
        }

        check_sum2<T>(result);
    }
  
    record_result( timer(), label );
}

/******************************************************************************/

template <typename T >
void test_for_loop_const4(const char *label) {
    int i;

    start_timer();
  
    for(i = 0; i < iterations; ++i) {
        T result = 0;
        int x, i;
        
        for (x = SIZE; x > 0; --x) {
            T temp = 0;
            for (i = 7; i >= 0; --i) {
                temp += T(1L << i);
            }
            result += temp;
        }

        check_sum2<T>(result);
    }
  
    record_result( timer(), label );
}

/******************************************************************************/

template <typename T >
void test_while_loop_const(const char *label) {
    int i;

    start_timer();
  
    for(i = 0; i < iterations; ++i) {
        T result = 0;
        int x, i;
        
        for (x = 0; x < SIZE; ++x) {
            T temp = 0;
            i = 0;
            while ( i < 8 ) {
                temp += T(1L << i);
                ++i;
            }
            result += temp;
        }

        check_sum2<T>(result);
    }
  
    record_result( timer(), label );
}

/******************************************************************************/

template <typename T >
void test_do_loop_const(const char *label) {
    int i;

    start_timer();
  
    for(i = 0; i < iterations; ++i) {
        T result = 0;
        int x, i;
        
        for (x = 0; x < SIZE; ++x) {
            T temp = 0;
            i = 0;
            do {
                temp += T(1L << i);
                ++i;
            } while ( i < 8 );
            
            result += temp;
        }

        check_sum2<T>(result);
    }
  
    record_result( timer(), label );
}

/******************************************************************************/

template <typename T >
void test_goto_loop_const(const char *label) {
    int i;

    start_timer();
  
    for(i = 0; i < iterations; ++i) {
        T result = 0;
        int x, i;
        
        for (x = 0; x < SIZE; ++x) {
            T temp = 0;
            i = 0;
            {
loop_start:
                temp += T(1L << i);
                ++i;
                
                if (i < 8)
                    goto loop_start;
            }
            
            result += temp;
        }

        check_sum2<T>(result);
    }
  
    record_result( timer(), label );
}

/******************************************************************************/
/******************************************************************************/

template <typename T >
void test_loop_empty_opt(const char *label) {
    int i;

    start_timer();
  
    for(i = 0; i < iterations; ++i) {
        T result = 0;
                
        result = (T)SIZE * (T)255;

        check_sum2<T>(result);
    }
  
    record_result( timer(), label );
}

/******************************************************************************/

template <typename T >
void test_for_loop_empty(const char *label) {
    int i;

    start_timer();
  
    for(i = 0; i < iterations; ++i) {
        T result = 0;
        int x;
        
        for (x = 0; x < SIZE; ++x) {
        }
        for (x = 0; x < SIZE; ++x) {
        }
        for (x = 0; x < SIZE; ++x) {
        }
        for (x = 0; x < SIZE; ++x) {
        }
        for (x = 0; x < SIZE; ++x) {
        }
        for (x = 0; x < SIZE; ++x) {
        }
        for (x = 0; x < SIZE; ++x) {
        }
        for (x = 0; x < SIZE; ++x) {
        }
        
        result = (T)SIZE * (T)255;

        check_sum2<T>(result);
    }
  
    record_result( timer(), label );
}

/******************************************************************************/

// reverse loop direction
template <typename T >
void test_for_loop_empty2(const char *label) {
    int i;

    start_timer();
  
    for(i = 0; i < iterations; ++i) {
        T result = 0;
        int x;
        
        for (x = SIZE; x > 0; --x) {
        }
        for (x = SIZE; x > 0; --x) {
        }
        for (x = SIZE; x > 0; --x) {
        }
        for (x = SIZE; x > 0; --x) {
        }
        for (x = SIZE; x > 0; --x) {
        }
        for (x = SIZE; x > 0; --x) {
        }
        for (x = SIZE; x > 0; --x) {
        }
        for (x = SIZE; x > 0; --x) {
        }
        
        result = (T)SIZE * (T)255;

        check_sum2<T>(result);
    }
  
    record_result( timer(), label );
}

/******************************************************************************/

template <typename T >
void test_while_loop_empty(const char *label) {
    int i;

    start_timer();
  
    for(i = 0; i < iterations; ++i) {
        T result = 0;
        int x;
        
        x = 0;
        while (x < SIZE) {
            ++x;
        }
        x = 0;
        while (x < SIZE) {
            ++x;
        }
        x = 0;
        while (x < SIZE) {
            ++x;
        }
        x = 0;
        while (x < SIZE) {
            ++x;
        }
        x = 0;
        while (x < SIZE) {
            ++x;
        }
        x = 0;
        while (x < SIZE) {
            ++x;
        }
        x = 0;
        while (x < SIZE) {
            ++x;
        }
        x = 0;
        while (x < SIZE) {
            ++x;
        }
        
        result = (T)SIZE * (T)255;

        check_sum2<T>(result);
    }
  
    record_result( timer(), label );
}

/******************************************************************************/

template <typename T >
void test_do_loop_empty(const char *label) {
    int i;

    start_timer();
  
    for(i = 0; i < iterations; ++i) {
        T result = 0;
        int x;
        
        x = 0;
        if (SIZE > 0)
        do {
            ++x;
        } while (x < SIZE);
        x = 0;
        if (SIZE > 0)
        do {
            ++x;
        } while (x < SIZE);
        x = 0;
        if (SIZE > 0)
        do {
            ++x;
        } while (x < SIZE);
        x = 0;
        if (SIZE > 0)
        do {
            ++x;
        } while (x < SIZE);
        x = 0;
        if (SIZE > 0)
        do {
            ++x;
        } while (x < SIZE);
        x = 0;
        if (SIZE > 0)
        do {
            ++x;
        } while (x < SIZE);
        x = 0;
        if (SIZE > 0)
        do {
            ++x;
        } while (x < SIZE);
        x = 0;
        if (SIZE > 0)
        do {
            ++x;
        } while (x < SIZE);
        
        result = (T)SIZE * (T)255;

        check_sum2<T>(result);
    }
  
    record_result( timer(), label );
}

/******************************************************************************/

template <typename T >
void test_goto_loop_empty(const char *label) {
    int i;

    start_timer();
  
    for(i = 0; i < iterations; ++i) {
        T result = 0;
        int x;
        
        x = 0;
        if (SIZE > 0)
        {
loop_start1:
            ++x;
            if (x < SIZE)
                goto loop_start1;
        }
        x = 0;
        if (SIZE > 0)
        {
loop_start2:
            ++x;
            if (x < SIZE)
                goto loop_start2;
        }
        x = 0;
        if (SIZE > 0)
        {
loop_start3:
            ++x;
            if (x < SIZE)
                goto loop_start3;
        }
        x = 0;
        if (SIZE > 0)
        {
loop_start4:
            ++x;
            if (x < SIZE)
                goto loop_start4;
        }
        x = 0;
        {
loop_start5:
            ++x;
            if (x < SIZE)
                goto loop_start5;
        }
        x = 0;
        if (SIZE > 0)
        {
loop_start6:
            ++x;
            if (x < SIZE)
                goto loop_start6;
        }
        x = 0;
        if (SIZE > 0)
        {
loop_start7:
            ++x;
            if (x < SIZE)
                goto loop_start7;
        }
        x = 0;
        if (SIZE > 0)
        {
loop_start8:
            ++x;
            if (x < SIZE)
                goto loop_start8;
        }
        
        result = (T)SIZE * (T)255;

        check_sum2<T>(result);
    }
  
    record_result( timer(), label );
}

/******************************************************************************/
/******************************************************************************/

template <typename T >
void test_for_loop_dead(const char *label) {
    int i;

    start_timer();
  
    for(i = 0; i < iterations; ++i) {
        T result = 0;
        T temp = 0;
        int x;
        
        for (x = 0; x < SIZE; ++x) {
            ++temp;
        }
        for (x = 0; x < SIZE; ++x) {
            temp += 3;
        }
        for (x = 0; x < SIZE; ++x) {
            temp ^= 0xAA;
        }
        for (x = 0; x < SIZE; ++x) {
            ++temp;
        }
        for (x = 0; x < SIZE; ++x) {
            temp -= 2;
        }
        for (x = 0; x < SIZE; ++x) {
            temp &= 0x55;
        }
        for (x = 0; x < SIZE; ++x) {
            ++temp;
        }
        for (x = 0; x < SIZE; ++x) {
            temp -= 7;
        }
        
        result = (T)SIZE * (T)255;

        check_sum2<T>(result);
    }
  
    record_result( timer(), label );
}
/******************************************************************************/

template <typename T >
void test_for_loop_dead2(const char *label) {
    int i;

    start_timer();
  
    for(i = 0; i < iterations; ++i) {
        T result = 0;
        T temp = 0;
        int x;
        
        for (x = SIZE; x > 0; --x) {
            ++temp;
        }
        for (x = SIZE; x > 0; --x) {
            temp += 3;
        }
        for (x = SIZE; x > 0; --x) {
            temp ^= 0xAA;
        }
        for (x = SIZE; x > 0; --x) {
            ++temp;
        }
        for (x = SIZE; x > 0; --x) {
            temp -= 2;
        }
        for (x = SIZE; x > 0; --x) {
            temp &= 0x55;
        }
        for (x = SIZE; x > 0; --x) {
            ++temp;
        }
        for (x = SIZE; x > 0; --x) {
            temp -= 7;
        }
        
        result = (T)SIZE * (T)255;

        check_sum2<T>(result);
    }
  
    record_result( timer(), label );
}

/******************************************************************************/

template <typename T >
void test_for_loop_dead3(const char *label) {
    int i;
    T result = 0;

    start_timer();
  
    for(i = 0; i < iterations; ++i) {
        int x;
        
        for (x = 0; x < SIZE; ++x) {
            result = x * 9;
        }
        for (x = 0; x < SIZE; ++x) {
            result = x * 11;
        }
        for (x = 0; x < SIZE; ++x) {
            result = x + 5;
        }
        for (x = 0; x < SIZE; ++x) {
            result += x ^ 0x55;
        }
        for (x = 0; x < SIZE; ++x) {
            result ^= x | 0x55;
        }
        for (x = 0; x < SIZE; ++x) {
            result |= x & 0x55;
        }
        for (x = 0; x < SIZE; ++x) {
            result += (x * 11) ^ 0x55;
        }
        for (x = 0; x < SIZE; ++x) {
            result += ((x * 13) / 7) ^ 0xAA;
        }
        
        result = (T)SIZE * (T)255;

        check_sum2<T>(result);
    }
  
    record_result( timer(), label );
}

/******************************************************************************/

template <typename T >
void test_while_loop_dead(const char *label) {
    int i;

    start_timer();
  
    for(i = 0; i < iterations; ++i) {
        T result = 0;
        T temp = 0;
        int x;
        
        x = 0;
        while (x < SIZE) {
            ++temp;
            ++x;
        }
        x = 0;
        while (x < SIZE) {
            temp += 3;
            ++x;
        }
        x = 0;
        while (x < SIZE) {
            ++temp;
            ++x;
        }
        x = 0;
        while (x < SIZE) {
            ++temp;
            ++x;
        }
        x = 0;
        while (x < SIZE) {
            temp -= 2;
            ++x;
        }
        x = 0;
        while (x < SIZE) {
            ++temp;
            ++x;
        }
        x = 0;
        while (x < SIZE) {
            ++temp;
            ++x;
        }
        x = 0;
        while (x < SIZE) {
            temp -= 7;
            ++x;
        }
        
        result = (T)SIZE * (T)255;

        check_sum2<T>(result);
    }
  
    record_result( timer(), label );
}

/******************************************************************************/

template <typename T >
void test_do_loop_dead(const char *label) {
    int i;

    start_timer();
  
    for(i = 0; i < iterations; ++i) {
        T result = 0;
        T temp = 0;
        int x;
        
        x = 0;
        if (SIZE > 0)
        do {
            ++temp;
            ++x;
        } while (x < SIZE);
        x = 0;
        if (SIZE > 0)
        do {
            temp += 3;
            ++x;
        } while (x < SIZE);
        x = 0;
        if (SIZE > 0)
        do {
            ++temp;
            ++x;
        } while (x < SIZE);
        x = 0;
        if (SIZE > 0)
        do {
            ++temp;
            ++x;
        } while (x < SIZE);
        x = 0;
        if (SIZE > 0)
        do {
            temp -= 2;
            ++x;
        } while (x < SIZE);
        x = 0;
        if (SIZE > 0)
        do {
            ++temp;
            ++x;
        } while (x < SIZE);
        x = 0;
        if (SIZE > 0)
        do {
            ++temp;
            ++x;
        } while (x < SIZE);
        x = 0;
        if (SIZE > 0)
        do {
            temp -= 7;
            ++x;
        } while (x < SIZE);
        
        result = (T)SIZE * (T)255;

        check_sum2<T>(result);
    }
  
    record_result( timer(), label );
}

/******************************************************************************/

template <typename T >
void test_goto_loop_dead(const char *label) {
    int i;

    start_timer();
  
    for(i = 0; i < iterations; ++i) {
        T result = 0;
        T temp = 0;
        int x;
        
        x = 0;
        if (SIZE > 0)
        {
loop_start1:
            ++temp;
            ++x;
            if (x < SIZE)
                goto loop_start1;
        }
        x = 0;
        if (SIZE > 0)
        {
loop_start2:
            temp += 3;
            ++x;
            if (x < SIZE)
                goto loop_start2;
        }
        x = 0;
        if (SIZE > 0)
        {
loop_start3:
            ++temp;
            ++x;
            if (x < SIZE)
                goto loop_start3;
        }
        x = 0;
        if (SIZE > 0)
        {
loop_start4:
            ++temp;
            ++x;
            if (x < SIZE)
                goto loop_start4;
        }
        x = 0;
        if (SIZE > 0)
        {
loop_start5:
            temp -= 2;
            ++x;
            if (x < SIZE)
                goto loop_start5;
        }
        x = 0;
        if (SIZE > 0)
        {
loop_start6:
            ++temp;
            ++x;
            if (x < SIZE)
                goto loop_start6;
        }
        x = 0;
        if (SIZE > 0)
        {
loop_start7:
            ++temp;
            ++x;
            if (x < SIZE)
                goto loop_start7;
        }
        x = 0;
        if (SIZE > 0)
        {
loop_start8:
            temp -= 7;
            ++x;
            if (x < SIZE)
                goto loop_start8;
        }
        
        result = (T)SIZE * (T)255;

        check_sum2<T>(result);
    }
  
    record_result( timer(), label );
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
    if (argc > 2) init_value = (double) atof(argv[2]);
    if (argc > 3) count = (int) atoi(argv[3]);
    


// int32_t
    test_loop_empty_opt<int32_t>( "int32_t loop removal empty optimal");
    test_for_loop_empty<int32_t>( "int32_t for loop removal empty");
    test_for_loop_empty2<int32_t>( "int32_t for loop removal empty reverse");
    test_while_loop_empty<int32_t>( "int32_t while loop removal empty");
    test_do_loop_empty<int32_t>( "int32_t do loop removal empty");
    test_goto_loop_empty<int32_t>( "int32_t goto loop removal empty");
    
    summarize("int32_t empty loop removal", SIZE, iterations, kDontShowGMeans, kDontShowPenalty );
    
    
    test_loop_empty_opt<int32_t>( "int32_t loop removal dead optimal");
    test_for_loop_dead<int32_t>( "int32_t for loop removal dead");
    test_for_loop_dead2<int32_t>( "int32_t for loop removal dead reverse");
    test_for_loop_dead3<int32_t>( "int32_t for loop removal dead assign");
    test_while_loop_dead<int32_t>( "int32_t while loop removal dead");
    test_do_loop_dead<int32_t>( "int32_t do loop removal dead");
    test_goto_loop_dead<int32_t>( "int32_t goto loop removal dead");
    
    summarize("int32_t dead loop removal", SIZE, iterations, kDontShowGMeans, kDontShowPenalty );


    test_loop_const_opt<int32_t>( "int32_t loop removal const optimal");
    test_for_loop_const<int32_t>( "int32_t for loop removal const");
    test_for_loop_const2<int32_t>( "int32_t for loop removal const reverse");
    test_for_loop_const3<int32_t>( "int32_t for loop removal const reverse bit");
    test_for_loop_const4<int32_t>( "int32_t for loop removal const reverse2");
    test_while_loop_const<int32_t>( "int32_t while loop removal const");
    test_do_loop_const<int32_t>( "int32_t do loop removal const");
    test_goto_loop_const<int32_t>( "int32_t goto loop removal const");
    
    summarize("int32_t const loop removal", SIZE, iterations, kDontShowGMeans, kDontShowPenalty );
    
    
    test_loop_opt<int32_t>( count, "int32_t loop removal variable optimal");
    test_for_loop_single<int32_t>( count, "int32_t for loop removal variable single");
    test_for_loop_single2<int32_t>( count, "int32_t for loop removal variable single reverse");
    test_for_loop_single3<int32_t>( count, "int32_t for loop removal variable single mixed");
    test_while_loop_single<int32_t>( count, "int32_t while loop removal variable single");
    test_do_loop_single<int32_t>( count, "int32_t do loop removal variable single");
    test_goto_loop_single<int32_t>( count, "int32_t goto loop removal variable single");
    
    
    ::fill( count_array, count_array+8, count );
    test_for_loop_multiple<int32_t>( count_array, "int32_t for loop removal variable multiple");
    test_for_loop_multiple2<int32_t>( count_array, "int32_t for loop removal variable multiple reverse");
    test_for_loop_multiple3<int32_t>( count_array, "int32_t for loop removal variable multiple mixed");
    test_while_loop_multiple<int32_t>( count_array, "int32_t while loop removal variable multiple");
    test_do_loop_multiple<int32_t>( count_array, "int32_t do loop removal variable multiple");
    test_goto_loop_multiple<int32_t>( count_array, "int32_t goto loop removal variable multiple");
    
    summarize("int32_t variable loop removal", (count*count)*(count*count)*(count*count)*(count*count), iterations, kDontShowGMeans, kDontShowPenalty );



    return 0;
}

// the end
/******************************************************************************/
/******************************************************************************/
