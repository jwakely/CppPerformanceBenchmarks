/*
     Copyright 2007-2008 Adobe Systems Incorporated
     Copyright 2019 Chris Cox
     Distributed under the MIT License (see accompanying file LICENSE_1_0_0.txt
     or a copy at http://stlab.adobe.com/licenses.html )
 
 
 Goal:  Test compiler optimizations related to invariant branches in loops (unswitching).
 
 Assumptions:
 
     1) The compiler will optimize loop invariant branches inside the loop body,
        usually moving them outside the loop.
         aka: loop unswitching
         for() {}
         while() {}
         do {} while()
         goto
 
 
 
 NOTE - can't always tell if unswitching worked, or just correctly avoids execution of unused values plus branch prediction
 
 */

#include "benchmark_stdint.hpp"
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <deque>
#include "benchmark_results.h"
#include "benchmark_timer.h"
#include "benchmark_typenames.h"

/******************************************************************************/

// this constant may need to be adjusted to give reasonable minimum times
// For best results, times should be about 1.0 seconds for the minimum test run
int iterations = 3000000;

// 8000 items, or between 8k and 64k of data
// this is intended to remain within the L2 cache of most common CPUs
#define SIZE     8000

// initial value for filling our arrays, may be changed from the command line
double init_value = 7.0;

/******************************************************************************/

#include "benchmark_shared_tests.h"

/******************************************************************************/

std::deque<std::string> gLabels;

/******************************************************************************/

template <typename T>
inline void check_sum(T result, const std::string &label) {
    T temp = (T)SIZE * (T)init_value;
    if (!tolerance_equal<T>(result,temp))
        printf("test %s failed\n", label.c_str() );
}

/******************************************************************************/
/******************************************************************************/

template <typename T>
T test_for_loop_opt(const T* first, int count, T test) {
    T result = 0;
    int n;
    
    for (n = 0; n < count; ++n) {
        result += first[n];
    }
    
    return result;
}

/******************************************************************************/

template <typename T >
T test_for_loop_param(const T* first, int count, T test) {
    T result = 0;
    int n;
    
    for (n = 0; n < count; ++n) {
        T temp = custom_multiple_constant_divide<T>::do_shift(first[n]);
        result += first[n];
        if (test < 0) {
            result += temp + custom_constant_divide<T>::do_shift(first[n]);
        }
    }
    return result;
}

/******************************************************************************/

template <typename T >
T test_for_loop_param2(const T* first, int count, T test) {
    T result = 0;
    int n;
    
    for (n = 0; n < count; ++n) {
        result += first[n];
        if (test < 0) {
            result += custom_constant_divide<T>::do_shift(first[n]);
        }
    }
    return result;
}

/******************************************************************************/

template <typename T >
T test_for_loop_global(const T* first, int count, T test) {
    T result = 0;
    int n;
    
    for (n = 0; n < count; ++n) {
        T temp = custom_multiple_constant_divide<T>::do_shift(first[n]);
        result += first[n];
        if (T(init_value) < 0) {
            result += temp + custom_constant_divide<T>::do_shift(first[n]);
        }
    }
    return result;
}

/******************************************************************************/

template <typename T >
T test_for_loop_global2(const T* first, int count, T test) {
    T result = 0;
    int n;
    
    for (n = 0; n < count; ++n) {
        result += first[n];
        if (T(init_value) < 0) {
            result += custom_constant_divide<T>::do_shift(first[n]);
        }
    }
    return result;
}

/******************************************************************************/

template <typename T >
T test_while_loop_opt(const T* first, int count, T test) {
    T result = 0;
    int n = 0;
    
    while ( n < count ) {
        result += first[n];
        ++n;
    }
    return result;
}

/******************************************************************************/

template <typename T >
T test_while_loop_param(const T* first, int count, T test) {
    T result = 0;
    int n = 0;
    
    while ( n < count ) {
        T temp = custom_multiple_constant_divide<T>::do_shift(first[n]);
        result += first[n];
        if (test < 0) {
            result += temp + custom_constant_divide<T>::do_shift(first[n]);
        }
        ++n;
    }
    return result;
}

/******************************************************************************/

template <typename T >
T test_while_loop_param2(const T* first, int count, T test) {
    T result = 0;
    int n = 0;
    
    while ( n < count ) {
        result += first[n];
        if (test < 0) {
            result += custom_constant_divide<T>::do_shift(first[n]);
        }
        ++n;
    }
    return result;
}

/******************************************************************************/

template <typename T >
T test_while_loop_global(const T* first, int count, T test) {
    T result = 0;
    int n = 0;
    
    while ( n < count ) {
        T temp = custom_multiple_constant_divide<T>::do_shift(first[n]);
        result += first[n];
        if (T(init_value) < 0) {
            result += temp + custom_constant_divide<T>::do_shift(first[n]);
        }
        ++n;
    }
    return result;
}

/******************************************************************************/

template <typename T >
T test_while_loop_global2(const T* first, int count, T test) {
    T result = 0;
    int n = 0;
    
    while ( n < count ) {
        result += first[n];
        if (T(init_value) < 0) {
            result += custom_constant_divide<T>::do_shift(first[n]);
        }
        ++n;
    }
    return result;
}

/******************************************************************************/

template <typename T >
T test_do_loop_opt(const T* first, int count, T test) {
    T result = 0;
    int n = 0;
    
    if (n < count)
        do {
            result += first[n];
            ++n;
        } while (n < count);
    
    return result;
}

/******************************************************************************/

template <typename T >
T test_do_loop_param(const T* first, int count, T test) {
    T result = 0;
    int n = 0;
    
    if (n < count)
        do {
            T temp = custom_multiple_constant_divide<T>::do_shift(first[n]);
            result += first[n];
            if (test < 0) {
                result += temp + custom_constant_divide<T>::do_shift(first[n]);
            }
            ++n;
        } while (n < count);
    
    return result;
}

/******************************************************************************/

template <typename T >
T test_do_loop_param2(const T* first, int count, T test) {
    T result = 0;
    int n = 0;
    
    if (n < count)
        do {
            result += first[n];
            if (test < 0) {
                result += custom_constant_divide<T>::do_shift(first[n]);
            }
            ++n;
        } while (n < count);
    
    return result;
}

/******************************************************************************/

template <typename T >
T test_do_loop_global(const T* first, int count, T test) {
    T result = 0;
    int n = 0;
    
    if (n < count)
        do {
            T temp = custom_multiple_constant_divide<T>::do_shift(first[n]);
            result += first[n];
            if (T(init_value) < 0) {
                result += temp + custom_constant_divide<T>::do_shift(first[n]);
            }
            ++n;
        } while (n < count);
    
    return result;
}

/******************************************************************************/

template <typename T >
T test_do_loop_global2(const T* first, int count, T test) {
    T result = 0;
    int n = 0;
    
    if (n < count)
        do {
            result += first[n];
            if (T(init_value) < 0) {
                result += custom_constant_divide<T>::do_shift(first[n]);
            }
            ++n;
        } while (n < count);
    
    return result;
}

/******************************************************************************/

template <typename T >
T test_goto_loop_opt(const T* first, int count, T test) {
    T result = 0;
    int n = 0;
    
    if (n < count) {
    loop_start:
        result += first[n];
        ++n;
        
        if (n < count)
            goto loop_start;
    }
    
    return result;
}

/******************************************************************************/

template <typename T >
T test_goto_loop_param(const T* first, int count, T test) {
    T result = 0;
    int n = 0;
    
    if (n < count) {
    loop_start:
        T temp = custom_multiple_constant_divide<T>::do_shift(first[n]);
        result += first[n];
        if (test < 0) {
            result += temp + custom_constant_divide<T>::do_shift(first[n]);
        }
        ++n;
        
        if (n < count)
            goto loop_start;
    }
    
    return result;
}

/******************************************************************************/

template <typename T >
T test_goto_loop_param2(const T* first, int count, T test) {
    T result = 0;
    int n = 0;
    
    if (n < count) {
    loop_start:
        result += first[n];
        if (test < 0) {
            result += custom_constant_divide<T>::do_shift(first[n]);
        }
        ++n;
        
        if (n < count)
            goto loop_start;
    }
    
    return result;
}

/******************************************************************************/

template <typename T >
T test_goto_loop_global(const T* first, int count, T test) {
    T result = 0;
    int n = 0;
    
    if (n < count) {
    loop_start:
        T temp = custom_multiple_constant_divide<T>::do_shift(first[n]);
        result += first[n];
        if (T(init_value) < 0) {
            result += temp + custom_constant_divide<T>::do_shift(first[n]);
        }
        ++n;
        
        if (n < count)
            goto loop_start;
    }
    
    return result;
}

/******************************************************************************/

template <typename T >
T test_goto_loop_global2(const T* first, int count, T test) {
    T result = 0;
    int n = 0;
    
    if (n < count) {
    loop_start:
        result += first[n];
        if (T(init_value) < 0) {
            result += custom_constant_divide<T>::do_shift(first[n]);
        }
        ++n;
        
        if (n < count)
            goto loop_start;
    }
    
    return result;
}

/******************************************************************************/
/******************************************************************************/

template <typename T >
T test_for_loop2_param(const T* first, int count, T test) {
    T result = 0;
    int n;
    
    for (n = 0; n < count; ++n) {
        T temp = custom_multiple_constant_divide<T>::do_shift(first[n]);
        result += first[n];
        if ((custom_multiple_constant_divide<T>::do_shift(first[n]) > 0) && (test < 0)) {
            result += temp + custom_multiple_constant_divide<T>::do_shift(first[n]);
        }
    }
    return result;
}

/******************************************************************************/

template <typename T >
T test_for_loop2_param2(const T* first, int count, T test) {
    T result = 0;
    int n;
    
    for (n = 0; n < count; ++n) {
        result += first[n];
        if ((custom_multiple_constant_divide<T>::do_shift(first[n]) > 0) && (test < 0)) {
            result += custom_multiple_constant_divide<T>::do_shift(first[n]);
        }
    }
    return result;
}

/******************************************************************************/

template <typename T >
T test_for_loop2_global(const T* first, int count, T test) {
    T result = 0;
    int n;
    
    for (n = 0; n < count; ++n) {
        T temp = custom_multiple_constant_divide<T>::do_shift(first[n]);
        result += first[n];
        if ((custom_multiple_constant_divide<T>::do_shift(first[n]) > 0) && (T(init_value) < 0)) {
            result += temp + custom_multiple_constant_divide<T>::do_shift(first[n]);
        }
    }
    return result;
}

/******************************************************************************/

template <typename T >
T test_for_loop2_global2(const T* first, int count, T test) {
    T result = 0;
    int n;
    
    for (n = 0; n < count; ++n) {
        result += first[n];
        if ((custom_multiple_constant_divide<T>::do_shift(first[n]) > 0) && (T(init_value) < 0)) {
            result += custom_multiple_constant_divide<T>::do_shift(first[n]);
        }
    }
    return result;
}

/******************************************************************************/

template <typename T >
T test_while_loop2_param(const T* first, int count, T test) {
    T result = 0;
    int n = 0;
    
    while ( n < count ) {
        T temp = custom_multiple_constant_divide<T>::do_shift(first[n]);
        result += first[n];
        if ((custom_multiple_constant_divide<T>::do_shift(first[n]) > 0) && (test < 0)) {
            result += temp + custom_constant_divide<T>::do_shift(first[n]);
        }
        ++n;
    }
    return result;
}

/******************************************************************************/

template <typename T >
T test_while_loop2_param2(const T* first, int count, T test) {
    T result = 0;
    int n = 0;
    
    while ( n < count ) {
        result += first[n];
        if ((custom_multiple_constant_divide<T>::do_shift(first[n]) > 0) && (test < 0)) {
            result += custom_constant_divide<T>::do_shift(first[n]);
        }
        ++n;
    }
    return result;
}

/******************************************************************************/

template <typename T >
T test_while_loop2_global(const T* first, int count, T test) {
    T result = 0;
    int n = 0;
    
    while ( n < count ) {
        T temp = custom_multiple_constant_divide<T>::do_shift(first[n]);
        result += first[n];
        if ((custom_multiple_constant_divide<T>::do_shift(first[n]) > 0) && (T(init_value) < 0)) {
            result += temp + custom_constant_divide<T>::do_shift(first[n]);
        }
        ++n;
    }
    return result;
}

/******************************************************************************/

template <typename T >
T test_while_loop2_global2(const T* first, int count, T test) {
    T result = 0;
    int n = 0;
    
    while ( n < count ) {
        result += first[n];
        if ((custom_multiple_constant_divide<T>::do_shift(first[n]) > 0) && (T(init_value) < 0)) {
            result += custom_constant_divide<T>::do_shift(first[n]);
        }
        ++n;
    }
    return result;
}

/******************************************************************************/

template <typename T >
T test_do_loop2_param(const T* first, int count, T test) {
    T result = 0;
    int n = 0;
    
    if (n < count)
        do {
            T temp = custom_multiple_constant_divide<T>::do_shift(first[n]);
            result += first[n];
            if ((custom_multiple_constant_divide<T>::do_shift(first[n]) > 0) && (test < 0)) {
                result += temp + custom_constant_divide<T>::do_shift(first[n]);
            }
            ++n;
        } while (n < count);
    return result;
}

/******************************************************************************/

template <typename T >
T test_do_loop2_param2(const T* first, int count, T test) {
    T result = 0;
    int n = 0;
    
    if (n < count)
        do {
            result += first[n];
            if ((custom_multiple_constant_divide<T>::do_shift(first[n]) > 0) && (test < 0)) {
                result += custom_constant_divide<T>::do_shift(first[n]);
            }
            ++n;
        } while (n < count);
    return result;
}

/******************************************************************************/

template <typename T >
T test_do_loop2_global(const T* first, int count, T test) {
    T result = 0;
    int n = 0;
    
    if (n < count)
        do {
            T temp = custom_multiple_constant_divide<T>::do_shift(first[n]);
            result += first[n];
            if ((custom_multiple_constant_divide<T>::do_shift(first[n]) > 0) && (T(init_value) < 0)) {
                result += temp + custom_constant_divide<T>::do_shift(first[n]);
            }
            ++n;
        } while (n < count);
    return result;
}

/******************************************************************************/

template <typename T >
T test_do_loop2_global2(const T* first, int count, T test) {
    T result = 0;
    int n = 0;
    
    if (n < count)
        do {
            result += first[n];
            if ((custom_multiple_constant_divide<T>::do_shift(first[n]) > 0) && (T(init_value) < 0)) {
                result += custom_constant_divide<T>::do_shift(first[n]);
            }
            ++n;
        } while (n < count);
    return result;
}

/******************************************************************************/

template <typename T >
T test_goto_loop2_param(const T* first, int count, T test) {
    T result = 0;
    int n = 0;
    
    if (n < count) {
    loop_start:
        T temp = custom_multiple_constant_divide<T>::do_shift(first[n]);
        result += first[n];
        if ((custom_multiple_constant_divide<T>::do_shift(first[n]) > 0) && (test < 0)) {
            result += temp + custom_constant_divide<T>::do_shift(first[n]);
        }
        ++n;
        
        if (n < count)
            goto loop_start;
    }
    return result;
}

/******************************************************************************/

template <typename T >
T test_goto_loop2_param2(const T* first, int count, T test) {
    T result = 0;
    int n = 0;
    
    if (n < count) {
    loop_start:
        result += first[n];
        if ((custom_multiple_constant_divide<T>::do_shift(first[n]) > 0) && (test < 0)) {
            result += custom_constant_divide<T>::do_shift(first[n]);
        }
        ++n;
        
        if (n < count)
            goto loop_start;
    }
    return result;
}

/******************************************************************************/

template <typename T >
T test_goto_loop2_global(const T* first, int count, T test) {
    T result = 0;
    int n = 0;
    
    if (n < count) {
    loop_start:
        T temp = custom_multiple_constant_divide<T>::do_shift(first[n]);
        result += first[n];
        if ((custom_multiple_constant_divide<T>::do_shift(first[n]) > 0) && (T(init_value) < 0)) {
            result += temp + custom_constant_divide<T>::do_shift(first[n]);
        }
        ++n;
        
        if (n < count)
            goto loop_start;
    }
    return result;
}

/******************************************************************************/

template <typename T >
T test_goto_loop2_global2(const T* first, int count, T test) {
    T result = 0;
    int n = 0;
    
    if (n < count) {
    loop_start:
        result += first[n];
        if ((custom_multiple_constant_divide<T>::do_shift(first[n]) > 0) && (T(init_value) < 0)) {
            result += custom_constant_divide<T>::do_shift(first[n]);
        }
        ++n;
        
        if (n < count)
            goto loop_start;
    }
    return result;
}

/******************************************************************************/
/******************************************************************************/

template <typename T >
T test_for_loop3_opt(const T* first, int count, T block) {
    T result = 0;
    int n;
    
    if ( count >= (2*block) ) {
        for (n = 0; n < count; ++n) {
            result += first[n];
        }
    } else {
        for (n = 0; n < block && n < count; ++n) {
            result += first[n] + 1;
        }
        for ( ; n < (2*block) && n < count; ++n) {
            result += first[n] - 1;
        }
        for ( ; n < count; ++n) {
            result += first[n];
        }
    }
    return result;
}

/******************************************************************************/

template <typename T >
T test_for_loop3_halfopt(const T* first, int count, T block) {
    T result = 0;
    int n;
    
    for (n = 0; n < block && n < count; ++n) {
        result += first[n] + 1;
    }
    for ( ; n < (2*block) && n < count; ++n) {
        result += first[n] - 1;
    }
    for ( ; n < count; ++n) {
        result += first[n];
    }
    return result;
}

/******************************************************************************/

template <typename T >
T test_for_loop3_param(const T* first, int count, T block) {
    T result = 0;
    int n;
    
    for (n = 0; n < count; ++n) {
        if (n < block)
            result += first[n] + 1;
        else if (n < (2*block))
            result += first[n] - 1;
        else
            result += first[n];
    }
    return result;
}
/******************************************************************************/

template <typename T >
T test_while_loop3_opt(const T* first, int count, T block) {
    T result = 0;
    int n = 0;
    
    if ( count >= (2*block) ) {
        while ( n < count ) {
            result += first[n];
            ++n;
        }
    } else {
        while ( n < block && n < count ) {
            result += first[n] + 1;
            ++n;
        }
        while ( n < (2*block) && n < count ) {
            result += first[n] - 1;
            ++n;
        }
        while ( n < count ) {
            result += first[n];
            ++n;
        }
    }
    return result;
}

/******************************************************************************/

template <typename T >
T test_while_loop3_halfopt(const T* first, int count, T block) {
    T result = 0;
    int n = 0;
    
    while ( n < block && n < count ) {
        result += first[n] + 1;
        ++n;
    }
    while ( n < (2*block) && n < count ) {
        result += first[n] - 1;
        ++n;
    }
    while ( n < count ) {
        result += first[n];
        ++n;
    }
    
    return result;
}

/******************************************************************************/

template <typename T >
T test_while_loop3_param(const T* first, int count, T block) {
    T result = 0;
    int n = 0;
    
    while ( n < count ) {
        if (n < block)
            result += first[n] + 1;
        else if (n < (2*block))
            result += first[n] - 1;
        else
            result += first[n];
        ++n;
    }
    return result;
}

/******************************************************************************/

template <typename T >
T test_do_loop3_opt(const T* first, int count, T block) {
    T result = 0;
    int n = 0;
    
    if ( count >= (2*block) ) {
        do {
            result += first[n];
            ++n;
        } while ( n < count );
    } else {
        do {
            result += first[n] + 1;
            ++n;
        } while ( n < block );
        do {
            result += first[n] - 1;
            ++n;
        } while ( n < (2*block) );
        do {
            result += first[n];
            ++n;
        } while ( n < count );
    }
    
    return result;
}

/******************************************************************************/

template <typename T >
T test_do_loop3_halfopt(const T* first, int count, T block) {
    T result = 0;
    int n = 0;
    
    if (n < count) {
        do {
            result += first[n] + 1;
            ++n;
        } while ( n < block );
        do {
            result += first[n] - 1;
            ++n;
        } while ( n < (2*block) );
        do {
            result += first[n];
            ++n;
        } while ( n < count );
    }
    
    return result;
}

/******************************************************************************/

template <typename T >
T test_do_loop3_param(const T* first, int count, T block) {
    T result = 0;
    int n = 0;
    
    if (n < count)
        do {
            if (n < block)
                result += first[n] + 1;
            else if (n < (2*block))
                result += first[n] - 1;
            else
                result += first[n];
            ++n;
        } while ( n < count );
    
    return result;
}

/******************************************************************************/

template <typename T >
T test_goto_loop3_opt(const T* first, int count, T block) {
    T result = 0;
    int n = 0;
    
    if ( count >= (2*block) ) {
        if (n < count) {
        loop4_start:
            result += first[n];
            ++n;
            if (n < count)
                goto loop4_start;
        }
    } else {
        
        if (n < count) {
        loop1_start:
            result += first[n] + 1;
            ++n;
            if (n < block && n < count)
                goto loop1_start;
        }
        
        if (n < count) {
        loop2_start:
            result += first[n] - 1;
            ++n;
            if (n < (2*block) && n < count)
                goto loop2_start;
        }
        
        if (n < count) {
        loop3_start:
            result += first[n];
            ++n;
            if (n < count)
                goto loop3_start;
        }
    }
    
    return result;
}

/******************************************************************************/

template <typename T >
T test_goto_loop3_halfopt(const T* first, int count, T block) {
    T result = 0;
    int n = 0;
    
    if ( n < count ) {
    loop1_start:
        result += first[n] + 1;
        ++n;
        if ( n < block && n < count )
            goto loop1_start;
    }
    
    if ( n < count) {
    loop2_start:
        result += first[n] - 1;
        ++n;
        if ( n < (2*block) && n < count )
            goto loop2_start;
    }
    
    if (n < count) {
    loop3_start:
        result += first[n];
        ++n;
        if ( n < count )
            goto loop3_start;
    }
    
    return result;
}

/******************************************************************************/

template <typename T >
T test_goto_loop3_param(const T* first, int count, T block) {
    T result = 0;
    int n = 0;
    
    if (n < count) {
    loop_start:
        if (n < block)
            result += first[n] + 1;
        else if (n < (2*block))
            result += first[n] - 1;
        else
            result += first[n];
        ++n;
    
        if (n < count)
            goto loop_start;
    }
    
    return result;
}

/******************************************************************************/
/******************************************************************************/

template <typename T, typename L >
void test_one_loop(const T* first, int count, T test, L looper, const std::string &label) {
    
    start_timer();
    
    for( int i = 0; i < iterations; ++i ) {
        T result = looper(first,count,test);
        check_sum<T>(result,label);
    }
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}
/******************************************************************************/

template <typename T, typename L >
void test_one_loop3(const T* first, int count, int block, L looper, const std::string &label) {
    
    start_timer();
    
    for( int i = 0; i < iterations; ++i ) {
        T result = looper(first,count,block);
        check_sum<T>(result,label);
    }
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/
/******************************************************************************/

template <typename T>
void TestOneType()
{
    std::string myTypeName( getTypeName<T>() );
    
    T data[ SIZE ];
    
    ::fill(data, data+SIZE, T(init_value));
    
    gLabels.clear();


    test_one_loop( data, SIZE, T(init_value), test_for_loop_opt<T>, myTypeName + " for unswitch optimal" );
    test_one_loop( data, SIZE, T(init_value), test_while_loop_opt<T>, myTypeName + " while unswitch optimal" );
    test_one_loop( data, SIZE, T(init_value), test_do_loop_opt<T>, myTypeName + " do unswitch optimal" );
    test_one_loop( data, SIZE, T(init_value), test_goto_loop_opt<T>, myTypeName + " goto unswitch optimal" );
    
    test_one_loop( data, SIZE, T(init_value), test_for_loop_param<T>, myTypeName + " for unswitch parameter" );
    test_one_loop( data, SIZE, T(init_value), test_for_loop_param2<T>, myTypeName + " for unswitch parameter2" );
    test_one_loop( data, SIZE, T(init_value), test_while_loop_param<T>, myTypeName + " while unswitch parameter" );
    test_one_loop( data, SIZE, T(init_value), test_while_loop_param2<T>, myTypeName + " while unswitch parameter2" );
    test_one_loop( data, SIZE, T(init_value), test_do_loop_param<T>, myTypeName + " do unswitch parameter" );
    test_one_loop( data, SIZE, T(init_value), test_do_loop_param2<T>, myTypeName + " do unswitch parameter2" );
    test_one_loop( data, SIZE, T(init_value), test_goto_loop_param<T>, myTypeName + " goto unswitch parameter" );
    test_one_loop( data, SIZE, T(init_value), test_goto_loop_param2<T>, myTypeName + " goto unswitch parameter2" );
    
    test_one_loop( data, SIZE, T(init_value), test_for_loop_global<T>, myTypeName + " for unswitch global" );
    test_one_loop( data, SIZE, T(init_value), test_for_loop_global2<T>, myTypeName + " for unswitch global2" );
    test_one_loop( data, SIZE, T(init_value), test_while_loop_global<T>, myTypeName + " while unswitch global" );
    test_one_loop( data, SIZE, T(init_value), test_while_loop_global2<T>, myTypeName + " while unswitch global2" );
    test_one_loop( data, SIZE, T(init_value), test_do_loop_global<T>, myTypeName + " do unswitch global" );
    test_one_loop( data, SIZE, T(init_value), test_do_loop_global2<T>, myTypeName + " do unswitch global2" );
    test_one_loop( data, SIZE, T(init_value), test_goto_loop_global<T>, myTypeName + " goto unswitch global" );
    test_one_loop( data, SIZE, T(init_value), test_goto_loop_global2<T>, myTypeName + " goto unswitch global2" );
    
    std::string temp1( myTypeName + " loop unswitching" );
    summarize(temp1.c_str(), SIZE, iterations, kDontShowGMeans, kDontShowPenalty );
    


    test_one_loop( data, SIZE, T(init_value), test_for_loop_opt<T>, myTypeName + " for unswitch2 optimal" );
    test_one_loop( data, SIZE, T(init_value), test_while_loop_opt<T>, myTypeName + " while unswitch2 optimal" );
    test_one_loop( data, SIZE, T(init_value), test_do_loop_opt<T>, myTypeName + " do unswitch2 optimal" );
    test_one_loop( data, SIZE, T(init_value), test_goto_loop_opt<T>, myTypeName + " goto unswitch2 optimal" );
    
    test_one_loop( data, SIZE, T(init_value), test_for_loop2_param<T>, myTypeName + " for unswitch2 parameter" );
    test_one_loop( data, SIZE, T(init_value), test_for_loop2_param2<T>, myTypeName + " for unswitch2 parameter2" );
    test_one_loop( data, SIZE, T(init_value), test_while_loop2_param<T>, myTypeName + " while unswitch2 parameter" );
    test_one_loop( data, SIZE, T(init_value), test_while_loop2_param2<T>, myTypeName + " while unswitch2 parameter2" );
    test_one_loop( data, SIZE, T(init_value), test_do_loop2_param<T>, myTypeName + " do unswitch2 parameter" );
    test_one_loop( data, SIZE, T(init_value), test_do_loop2_param2<T>, myTypeName + " do unswitch2 parameter2" );
    test_one_loop( data, SIZE, T(init_value), test_goto_loop2_param<T>, myTypeName + " goto unswitch2 parameter" );
    test_one_loop( data, SIZE, T(init_value), test_goto_loop2_param2<T>, myTypeName + " goto unswitch2 parameter2" );
    
    test_one_loop( data, SIZE, T(init_value), test_for_loop2_global<T>, myTypeName + " for unswitch2 global" );
    test_one_loop( data, SIZE, T(init_value), test_for_loop2_global2<T>, myTypeName + " for unswitch2 global2" );
    test_one_loop( data, SIZE, T(init_value), test_while_loop2_global<T>, myTypeName + " while unswitch2 global" );
    test_one_loop( data, SIZE, T(init_value), test_while_loop2_global2<T>, myTypeName + " while unswitch2 global2" );
    test_one_loop( data, SIZE, T(init_value), test_do_loop2_global<T>, myTypeName + " do unswitch2 global" );
    test_one_loop( data, SIZE, T(init_value), test_do_loop2_global2<T>, myTypeName + " do unswitch2 global2" );
    test_one_loop( data, SIZE, T(init_value), test_goto_loop2_global<T>, myTypeName + " goto unswitch2 global" );
    test_one_loop( data, SIZE, T(init_value), test_goto_loop2_global2<T>, myTypeName + " goto unswitch2 global2" );
    
    std::string temp2( myTypeName + " loop unswitching2" );
    summarize(temp2.c_str(), SIZE, iterations, kDontShowGMeans, kDontShowPenalty );




    test_one_loop3( data, SIZE, SIZE/4, test_for_loop3_opt<T>, myTypeName + " for unswitch3 optimal" );
    test_one_loop3( data, SIZE, SIZE/4, test_for_loop3_halfopt<T>, myTypeName + " for unswitch3 half_opt" );
    test_one_loop3( data, SIZE, SIZE/4, test_while_loop3_opt<T>, myTypeName + " while unswitch3 optimal" );
    test_one_loop3( data, SIZE, SIZE/4, test_while_loop3_halfopt<T>, myTypeName + " while unswitch3 half_opt" );
    test_one_loop3( data, SIZE, SIZE/4, test_do_loop3_opt<T>, myTypeName + " do unswitch3 optimal" );
    test_one_loop3( data, SIZE, SIZE/4, test_do_loop3_halfopt<T>, myTypeName + " do unswitch3 half_opt" );
    test_one_loop3( data, SIZE, SIZE/4, test_goto_loop3_opt<T>, myTypeName + " goto unswitch3 optimal" );
    test_one_loop3( data, SIZE, SIZE/4, test_goto_loop3_halfopt<T>, myTypeName + " goto unswitch3 half_opt" );
    
    test_one_loop3( data, SIZE, SIZE/4, test_for_loop3_param<T>, myTypeName + " for unswitch3 parameter" );
    test_one_loop3( data, SIZE, SIZE/4, test_while_loop3_param<T>, myTypeName + " while unswitch3 parameter" );
    test_one_loop3( data, SIZE, SIZE/4, test_do_loop3_param<T>, myTypeName + " do unswitch3 parameter" );
    test_one_loop3( data, SIZE, SIZE/4, test_goto_loop3_param<T>, myTypeName + " goto unswitch3 parameter" );
    
    std::string temp3( myTypeName + " loop unswitching3" );
    summarize(temp3.c_str(), SIZE, iterations, kDontShowGMeans, kDontShowPenalty );

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
    
    
    // found some problems with do and goto loops for int16_t and int8_t
    TestOneType<int16_t>();
    
    iterations /= 2;
    TestOneType<int32_t>();
    
    iterations /= 4;
    TestOneType<double>();
    
    
#if TEST_ALL
    TestOneType<int8_t>();
    TestOneType<uint8_t>();
    
    //TestOneType<int16_t>();
    TestOneType<uint16_t>();
    
    iterations /= 2;
    //TestOneType<int32_t>();
    TestOneType<uint32_t>();
    
    iterations /= 2;
    TestOneType<int64_t>();
    TestOneType<uint64_t>();
    
    iterations /= 2;
    TestOneType<float>();
    //TestOneType<double>();
    TestOneType<long double>();
#endif
    
    
    return 0;
}

// the end
/******************************************************************************/
/******************************************************************************/
