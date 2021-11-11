/*
    Copyright 2008 Adobe Systems Incorporated
    Copyright 2019 Chris Cox
    Distributed under the MIT License (see accompanying file LICENSE_1_0_0.txt
    or a copy at http://stlab.adobe.com/licenses.html )


Goal:  Test compiler issues related to recursive template instantiation.

Assumptions:
    1) Unrolled template code should perform no worse than manually unrolled code.
 
    2) Binary, trinary, and higher order template instantiations should be just as efficient
        as linear recursion, and should be unrolled properly for simple operations.

    3) Template unrolling of loops should not hurt performance.
        See also loop_unroll.cpp



binary
trinary / ternary
quaternary
quinary
senary
septenary
octal
nonary
decimal


*/

#include "benchmark_stdint.hpp"
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <deque>
#include <string>
#include "benchmark_results.h"
#include "benchmark_timer.h"
#include "benchmark_typenames.h"

/******************************************************************************/

// this constant may need to be adjusted to give reasonable minimum times
// For best results, times should be about 1.0 seconds for the minimum test run
int iterations = 300000;

// 8000 items, or between 8k and 64k of data
// this is intended to remain within the L2 cache of most common CPUs
const int SIZE = 8000;

// initial value for filling our arrays, may be changed from the command line
double init_value = 2.0;

const int kUnrollLimit = 64;

std::deque<std::string> gLabels;

/******************************************************************************/

#include "benchmark_shared_tests.h"

/******************************************************************************/
/******************************************************************************/

template <typename T>
T hash_func_base(T seed) {
    return (914237 * (seed + 12345)) - 13;
}

template <typename T>
T complete_hash_func(T seed) {
    return hash_func_base( hash_func_base( seed ) );
}

/******************************************************************************/

template <typename T>
inline void check_sum(T result, const std::string &label) {
    T temp = (T)SIZE * complete_hash_func( (T)init_value );
    if (!tolerance_equal<T>(result,temp))
        printf("test %s failed\n", label.c_str());
}

/******************************************************************************/
/******************************************************************************/

template< int F, typename T >
struct loop_inner_body_linear {
    inline static void do_work(T &result, const T *first, int n) {
        loop_inner_body_linear<F-1,T>::do_work(result, first, n);
        T temp = first[ n + (F-1) ];
        temp = complete_hash_func( temp );
        result += temp;
    }
};

template< typename T >
struct loop_inner_body_linear<0,T> {
    inline static void do_work(T &, const T *, int) {
    }
};

/******************************************************************************/

template< int F, typename T >
struct loop_inner_body_binary {
    inline static void do_work(T &result, const T *first, int n) {
        loop_inner_body_binary<F/2,T>::do_work(result, first, n+0*(F/2));
        loop_inner_body_binary<F-(F/2),T>::do_work(result, first, n+1*(F/2));
    }
};

template< typename T >
struct loop_inner_body_binary<1,T> {
    inline static void do_work(T &result, const T *first, int n) {
        T temp = first[ n ];
        temp = complete_hash_func( temp );
        result += temp;
    }
};

template< typename T >
struct loop_inner_body_binary<0,T> {
    inline static void do_work(T &, const T *, int) {
    }
};

/******************************************************************************/

template< int F, typename T >
struct loop_inner_body_trinary {
    inline static void do_work(T &result, const T *first, int n) {
        if (F > 2) {
            loop_inner_body_trinary<F/3,T>::do_work(result, first, n+0*(F/3));
            loop_inner_body_trinary<F/3,T>::do_work(result, first, n+1*(F/3));
            loop_inner_body_trinary<F-2*(F/3),T>::do_work(result, first, n+2*(F/3));
        } else {
            // go linear when we get below the division size
            loop_inner_body_trinary<1,T>::do_work(result, first, n);
            loop_inner_body_trinary<F-1,T>::do_work(result, first, n+1);
        }
    }
};

template< typename T >
struct loop_inner_body_trinary<1,T> {
    inline static void do_work(T &result, const T *first, int n) {
        T temp = first[ n ];
        temp = complete_hash_func( temp );
        result += temp;
    }
};

template< typename T >
struct loop_inner_body_trinary<0,T> {
    inline static void do_work(T &, const T *, int) {
    }
};

/******************************************************************************/

template< int F, typename T >
struct loop_inner_body_quaternary {
    inline static void do_work(T &result, const T *first, int n) {
        if (F > 3) {
            loop_inner_body_quaternary<F/4,T>::do_work(result, first, n+0*(F/4));
            loop_inner_body_quaternary<F/4,T>::do_work(result, first, n+1*(F/4));
            loop_inner_body_quaternary<F/4,T>::do_work(result, first, n+2*(F/4));
            loop_inner_body_quaternary<F-3*(F/4),T>::do_work(result, first, n+3*(F/4));
        } else {
            // go linear when we get below the division size
            loop_inner_body_quaternary<1,T>::do_work(result, first, n);
            loop_inner_body_quaternary<F-1,T>::do_work(result, first, n+1);
        }
    }
};

template< typename T >
struct loop_inner_body_quaternary<1,T> {
    inline static void do_work(T &result, const T *first, int n) {
        T temp = first[ n ];
        temp = complete_hash_func( temp );
        result += temp;
    }
};

template< typename T >
struct loop_inner_body_quaternary<0,T> {
    inline static void do_work(T &, const T *, int) {
    }
};

/******************************************************************************/

template< int F, typename T >
struct loop_inner_body_octal {
    inline static void do_work(T &result, const T *first, int n) {
        if (F > 7) {
            loop_inner_body_octal<F/8,T>::do_work(result, first, n+0*(F/8));
            loop_inner_body_octal<F/8,T>::do_work(result, first, n+1*(F/8));
            loop_inner_body_octal<F/8,T>::do_work(result, first, n+2*(F/8));
            loop_inner_body_octal<F/8,T>::do_work(result, first, n+3*(F/8));
            loop_inner_body_octal<F/8,T>::do_work(result, first, n+4*(F/8));
            loop_inner_body_octal<F/8,T>::do_work(result, first, n+5*(F/8));
            loop_inner_body_octal<F/8,T>::do_work(result, first, n+6*(F/8));
            loop_inner_body_octal<F-(7*(F/8)),T>::do_work(result, first, n+7*(F/8));
        } else {
            // go linear when we get below the division size
            loop_inner_body_octal<1,T>::do_work(result, first, n);
            loop_inner_body_octal<F-1,T>::do_work(result, first, n+1);
        }
    }
};

template< typename T >
struct loop_inner_body_octal<1,T> {
    inline static void do_work(T &result, const T *first, int n) {
        T temp = first[ n ];
        temp = complete_hash_func( temp );
        result += temp;
    }
};

template< typename T >
struct loop_inner_body_octal<0,T> {
    inline static void do_work(T &, const T *, int) {
    }
};

/******************************************************************************/

template< int F, typename T >
struct loop_inner_body_decimal {
    inline static void do_work(T &result, const T *first, int n) {
        if (F > 9) {
            loop_inner_body_decimal<F/10,T>::do_work(result, first, n+0*(F/10));
            loop_inner_body_decimal<F/10,T>::do_work(result, first, n+1*(F/10));
            loop_inner_body_decimal<F/10,T>::do_work(result, first, n+2*(F/10));
            loop_inner_body_decimal<F/10,T>::do_work(result, first, n+3*(F/10));
            loop_inner_body_decimal<F/10,T>::do_work(result, first, n+4*(F/10));
            loop_inner_body_decimal<F/10,T>::do_work(result, first, n+5*(F/10));
            loop_inner_body_decimal<F/10,T>::do_work(result, first, n+6*(F/10));
            loop_inner_body_decimal<F/10,T>::do_work(result, first, n+7*(F/10));
            loop_inner_body_decimal<F/10,T>::do_work(result, first, n+8*(F/10));
            loop_inner_body_decimal<F-(9*(F/10)),T>::do_work(result, first, n+9*(F/10));
        } else {
            // go linear when we get below the division size
            loop_inner_body_decimal<1,T>::do_work(result, first, n);
            loop_inner_body_decimal<F-1,T>::do_work(result, first, n+1);
        }
    }
};

template< typename T >
struct loop_inner_body_decimal<1,T> {
    inline static void do_work(T &result, const T *first, int n) {
        T temp = first[ n ];
        temp = complete_hash_func( temp );
        result += temp;
    }
};

template< typename T >
struct loop_inner_body_decimal<0,T> {
    inline static void do_work(T &, const T *, int) {
    }
};

/******************************************************************************/
/******************************************************************************/

// F is the unrolling factor
template <int F, typename T >
void test_loop_unroll_linear(const T* first, int count, const std::string label) {
    int i;
    
    start_timer();
    
    for(i = 0; i < iterations; ++i) {
        T result = 0;
        int n = 0;
        
        for (; n < (count - (F-1)); n += F) {
            loop_inner_body_linear<F,T>::do_work(result,first, n);
        }
        
        for (; n < count; ++n) {
            result += complete_hash_func( first[n] );
        }
        
        check_sum<T>(result, label);
    }
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

// F is the unrolling factor
template <int F, typename T >
void test_loop_unroll_binary(const T* first, int count, const std::string label) {
    int i;
    
    start_timer();
    
    for(i = 0; i < iterations; ++i) {
        T result = 0;
        int n = 0;
        
        for (; n < (count - (F-1)); n += F) {
            loop_inner_body_binary<F,T>::do_work(result,first, n);
        }
        
        for (; n < count; ++n) {
            result += complete_hash_func( first[n] );
        }
        
        check_sum<T>(result,label);
    }
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

// F is the unrolling factor
template <int F, typename T >
void test_loop_unroll_trinary(const T* first, int count, const std::string label) {
    int i;
    
    start_timer();
    
    for(i = 0; i < iterations; ++i) {
        T result = 0;
        int n = 0;
        
        for (; n < (count - (F-1)); n += F) {
            loop_inner_body_trinary<F,T>::do_work(result,first, n);
        }
        
        for (; n < count; ++n) {
            result += complete_hash_func( first[n] );
        }
        
        check_sum<T>(result,label);
    }
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

// F is the unrolling factor
template <int F, typename T >
void test_loop_unroll_quaternary(const T* first, int count, const std::string label) {
    int i;
    
    start_timer();
    
    for(i = 0; i < iterations; ++i) {
        T result = 0;
        int n = 0;
        
        for (; n < (count - (F-1)); n += F) {
            loop_inner_body_quaternary<F,T>::do_work(result,first, n);
        }
        
        for (; n < count; ++n) {
            result += complete_hash_func( first[n] );
        }
        
        check_sum<T>(result,label);
    }
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

// F is the unrolling factor
template <int F, typename T >
void test_loop_unroll_octal(const T* first, int count, const std::string label) {
    int i;
    
    start_timer();
    
    for(i = 0; i < iterations; ++i) {
        T result = 0;
        int n = 0;
        
        for (; n < (count - (F-1)); n += F) {
            loop_inner_body_octal<F,T>::do_work(result,first, n);
        }
        
        for (; n < count; ++n) {
            result += complete_hash_func( first[n] );
        }
        
        check_sum<T>(result,label);
    }
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

// F is the unrolling factor
template <int F, typename T >
void test_loop_unroll_decimal(const T* first, int count, const std::string label) {
    int i;
    
    start_timer();
    
    for(i = 0; i < iterations; ++i) {
        T result = 0;
        int n = 0;
        
        for (; n < (count - (F-1)); n += F) {
            loop_inner_body_decimal<F,T>::do_work(result,first, n);
        }
        
        for (; n < count; ++n) {
            result += complete_hash_func( first[n] );
        }
        
        check_sum<T>(result,label);
    }
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/
/******************************************************************************/

// another unrolled loop to create all of our tests
template< int N, typename T >
struct loop_tests_linear {
    static void do_test( const T *data, std::string label_base) {
        loop_tests_linear<N-1, T>::do_test(data, label_base);
        std::string label = label_base + " " + std::to_string( N );
        test_loop_unroll_linear<N>( data, SIZE, label );
    }
};

template<typename T>
struct loop_tests_linear<0,T> {
    static void do_test( const T *data, const std::string label ) {
    }
};

/******************************************************************************/
/******************************************************************************/

// another unrolled loop to create all of our tests
template< int N, typename T >
struct loop_tests_binary {
    static void do_test( const T *data, std::string label_base) {
        loop_tests_binary<N-1, T>::do_test(data, label_base);
        std::string label = label_base + " " + std::to_string( N );
        test_loop_unroll_binary<N>( data, SIZE, label );
    }
};

template<typename T>
struct loop_tests_binary<0,T> {
    static void do_test( const T *data, const std::string label ) {
    }
};

/******************************************************************************/
/******************************************************************************/

// another unrolled loop to create all of our tests
template< int N, typename T >
struct loop_tests_trinary {
    static void do_test( const T *data, std::string label_base) {
        loop_tests_trinary<N-1, T>::do_test(data, label_base);
        std::string label = label_base + " " + std::to_string( N );
        test_loop_unroll_trinary<N>( data, SIZE, label );
    }
};

template<typename T>
struct loop_tests_trinary<0,T> {
    static void do_test( const T *data, const std::string label ) {
    }
};

/******************************************************************************/
/******************************************************************************/

// another unrolled loop to create all of our tests
template< int N, typename T >
struct loop_tests_quaternary {
    static void do_test( const T *data, std::string label_base) {
        loop_tests_quaternary<N-1, T>::do_test(data, label_base);
        std::string label = label_base + " " + std::to_string( N );
        test_loop_unroll_quaternary<N>( data, SIZE, label );
    }
};

template<typename T>
struct loop_tests_quaternary<0,T> {
    static void do_test( const T *data, const std::string label ) {
    }
};

/******************************************************************************/
/******************************************************************************/

// another unrolled loop to create all of our tests
template< int N, typename T >
struct loop_tests_octal {
    static void do_test( const T *data, std::string label_base) {
        loop_tests_octal<N-1, T>::do_test(data, label_base);
        std::string label = label_base + " " + std::to_string( N );
        test_loop_unroll_octal<N>( data, SIZE, label );
    }
};

template<typename T>
struct loop_tests_octal<0,T> {
    static void do_test( const T *data, const std::string label ) {
    }
};

/******************************************************************************/
/******************************************************************************/

// another unrolled loop to create all of our tests
template< int N, typename T >
struct loop_tests_decimal {
    static void do_test( const T *data, std::string label_base) {
        loop_tests_decimal<N-1, T>::do_test(data, label_base);
        std::string label = label_base + " " + std::to_string( N );
        test_loop_unroll_decimal<N>( data, SIZE, label );
    }
};

template<typename T>
struct loop_tests_decimal<0,T> {
    static void do_test( const T *data, const std::string label ) {
    }
};

/******************************************************************************/
/******************************************************************************/

template < typename T >
void test_loop_unroll_1(const T* first, int count, const std::string label) {
    int i;
    start_timer();
    for(i = 0; i < iterations; ++i) {
        T result = 0;
        int n = 0;
        for (; n < (count - (1-1)); n += 1) {
            {T temp = first[ n + 0 ]; temp = complete_hash_func( temp ); result += temp;}
        }
        for (; n < count; ++n) {
            result += complete_hash_func( first[n] );
        }
        check_sum<T>(result,label);
    }
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

template < typename T >
void test_loop_unroll_2(const T* first, int count, const std::string label) {
    int i;
    start_timer();
    for(i = 0; i < iterations; ++i) {
        T result = 0;
        int n = 0;
        for (; n < (count - (2-1)); n += 2) {
            {T temp = first[ n + 0 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 1 ]; temp = complete_hash_func( temp ); result += temp;}
        }
        for (; n < count; ++n) {
            result += complete_hash_func( first[n] );
        }
        check_sum<T>(result,label);
    }
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

template < typename T >
void test_loop_unroll_3(const T* first, int count, const std::string label) {
    int i;
    start_timer();
    for(i = 0; i < iterations; ++i) {
        T result = 0;
        int n = 0;
        for (; n < (count - (3-1)); n += 3) {
            {T temp = first[ n + 0 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 1 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 2 ]; temp = complete_hash_func( temp ); result += temp;}
        }
        for (; n < count; ++n) {
            result += complete_hash_func( first[n] );
        }
        check_sum<T>(result,label);
    }
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

template < typename T >
void test_loop_unroll_4(const T* first, int count, const std::string label) {
    int i;
    start_timer();
    for(i = 0; i < iterations; ++i) {
        T result = 0;
        int n = 0;
        for (; n < (count - (4-1)); n += 4) {
            {T temp = first[ n + 0 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 1 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 2 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 3 ]; temp = complete_hash_func( temp ); result += temp;}
        }
        for (; n < count; ++n) {
            result += complete_hash_func( first[n] );
        }
        check_sum<T>(result,label);
    }
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

template < typename T >
void test_loop_unroll_5(const T* first, int count, const std::string label) {
    int i;
    start_timer();
    for(i = 0; i < iterations; ++i) {
        T result = 0;
        int n = 0;
        for (; n < (count - (5-1)); n += 5) {
            {T temp = first[ n + 0 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 1 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 2 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 3 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 4 ]; temp = complete_hash_func( temp ); result += temp;}
        }
        for (; n < count; ++n) {
            result += complete_hash_func( first[n] );
        }
        check_sum<T>(result,label);
    }
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

template < typename T >
void test_loop_unroll_6(const T* first, int count, const std::string label) {
    int i;
    start_timer();
    for(i = 0; i < iterations; ++i) {
        T result = 0;
        int n = 0;
        for (; n < (count - (6-1)); n += 6) {
            {T temp = first[ n + 0 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 1 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 2 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 3 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 4 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 5 ]; temp = complete_hash_func( temp ); result += temp;}
        }
        for (; n < count; ++n) {
            result += complete_hash_func( first[n] );
        }
        check_sum<T>(result,label);
    }
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

template < typename T >
void test_loop_unroll_7(const T* first, int count, const std::string label) {
    int i;
    start_timer();
    for(i = 0; i < iterations; ++i) {
        T result = 0;
        int n = 0;
        for (; n < (count - (7-1)); n += 7) {
            {T temp = first[ n + 0 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 1 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 2 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 3 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 4 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 5 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 6 ]; temp = complete_hash_func( temp ); result += temp;}
        }
        for (; n < count; ++n) {
            result += complete_hash_func( first[n] );
        }
        check_sum<T>(result,label);
    }
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

template < typename T >
void test_loop_unroll_8(const T* first, int count, const std::string label) {
    int i;
    start_timer();
    for(i = 0; i < iterations; ++i) {
        T result = 0;
        int n = 0;
        for (; n < (count - (8-1)); n += 8) {
            {T temp = first[ n + 0 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 1 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 2 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 3 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 4 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 5 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 6 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 7 ]; temp = complete_hash_func( temp ); result += temp;}
        }
        for (; n < count; ++n) {
            result += complete_hash_func( first[n] );
        }
        check_sum<T>(result,label);
    }
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

template < typename T >
void test_loop_unroll_9(const T* first, int count, const std::string label) {
    int i;
    start_timer();
    for(i = 0; i < iterations; ++i) {
        T result = 0;
        int n = 0;
        for (; n < (count - (9-1)); n += 9) {
            {T temp = first[ n + 0 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 1 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 2 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 3 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 4 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 5 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 6 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 7 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 8 ]; temp = complete_hash_func( temp ); result += temp;}
        }
        for (; n < count; ++n) {
            result += complete_hash_func( first[n] );
        }
        check_sum<T>(result,label);
    }
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

template < typename T >
void test_loop_unroll_10(const T* first, int count, const std::string label) {
    int i;
    start_timer();
    for(i = 0; i < iterations; ++i) {
        T result = 0;
        int n = 0;
        for (; n < (count - (10-1)); n += 10) {
            {T temp = first[ n + 0 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 1 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 2 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 3 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 4 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 5 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 6 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 7 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 8 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 9 ]; temp = complete_hash_func( temp ); result += temp;}
        }
        for (; n < count; ++n) {
            result += complete_hash_func( first[n] );
        }
        check_sum<T>(result,label);
    }
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

template < typename T >
void test_loop_unroll_11(const T* first, int count, const std::string label) {
    int i;
    start_timer();
    for(i = 0; i < iterations; ++i) {
        T result = 0;
        int n = 0;
        for (; n < (count - (11-1)); n += 11) {
            {T temp = first[ n + 0 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 1 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 2 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 3 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 4 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 5 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 6 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 7 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 8 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 9 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 10 ]; temp = complete_hash_func( temp ); result += temp;}
        }
        for (; n < count; ++n) {
            result += complete_hash_func( first[n] );
        }
        check_sum<T>(result,label);
    }
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

template < typename T >
void test_loop_unroll_12(const T* first, int count, const std::string label) {
    int i;
    start_timer();
    for(i = 0; i < iterations; ++i) {
        T result = 0;
        int n = 0;
        for (; n < (count - (12-1)); n += 12) {
            {T temp = first[ n + 0 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 1 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 2 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 3 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 4 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 5 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 6 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 7 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 8 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 9 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 10 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 11 ]; temp = complete_hash_func( temp ); result += temp;}
        }
        for (; n < count; ++n) {
            result += complete_hash_func( first[n] );
        }
        check_sum<T>(result,label);
    }
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

template < typename T >
void test_loop_unroll_13(const T* first, int count, const std::string label) {
    int i;
    start_timer();
    for(i = 0; i < iterations; ++i) {
        T result = 0;
        int n = 0;
        for (; n < (count - (13-1)); n += 13) {
            {T temp = first[ n + 0 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 1 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 2 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 3 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 4 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 5 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 6 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 7 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 8 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 9 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 10 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 11 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 12 ]; temp = complete_hash_func( temp ); result += temp;}
        }
        for (; n < count; ++n) {
            result += complete_hash_func( first[n] );
        }
        check_sum<T>(result,label);
    }
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

template < typename T >
void test_loop_unroll_14(const T* first, int count, const std::string label) {
    int i;
    start_timer();
    for(i = 0; i < iterations; ++i) {
        T result = 0;
        int n = 0;
        for (; n < (count - (14-1)); n += 14) {
            {T temp = first[ n + 0 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 1 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 2 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 3 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 4 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 5 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 6 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 7 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 8 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 9 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 10 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 11 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 12 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 13 ]; temp = complete_hash_func( temp ); result += temp;}
        }
        for (; n < count; ++n) {
            result += complete_hash_func( first[n] );
        }
        check_sum<T>(result,label);
    }
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

template < typename T >
void test_loop_unroll_15(const T* first, int count, const std::string label) {
    int i;
    start_timer();
    for(i = 0; i < iterations; ++i) {
        T result = 0;
        int n = 0;
        for (; n < (count - (15-1)); n += 15) {
            {T temp = first[ n + 0 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 1 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 2 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 3 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 4 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 5 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 6 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 7 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 8 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 9 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 10 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 11 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 12 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 13 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 14 ]; temp = complete_hash_func( temp ); result += temp;}
        }
        for (; n < count; ++n) {
            result += complete_hash_func( first[n] );
        }
        check_sum<T>(result,label);
    }
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

template < typename T >
void test_loop_unroll_16(const T* first, int count, const std::string label) {
    int i;
    start_timer();
    for(i = 0; i < iterations; ++i) {
        T result = 0;
        int n = 0;
        for (; n < (count - (16-1)); n += 16) {
            {T temp = first[ n + 0 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 1 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 2 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 3 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 4 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 5 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 6 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 7 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 8 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 9 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 10 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 11 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 12 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 13 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 14 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 15 ]; temp = complete_hash_func( temp ); result += temp;}
        }
        for (; n < count; ++n) {
            result += complete_hash_func( first[n] );
        }
        check_sum<T>(result,label);
    }
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

template < typename T >
void test_loop_unroll_17(const T* first, int count, const std::string label) {
    int i;
    start_timer();
    for(i = 0; i < iterations; ++i) {
        T result = 0;
        int n = 0;
        for (; n < (count - (17-1)); n += 17) {
            {T temp = first[ n + 0 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 1 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 2 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 3 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 4 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 5 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 6 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 7 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 8 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 9 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 10 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 11 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 12 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 13 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 14 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 15 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 16 ]; temp = complete_hash_func( temp ); result += temp;}
        }
        for (; n < count; ++n) {
            result += complete_hash_func( first[n] );
        }
        check_sum<T>(result,label);
    }
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

template < typename T >
void test_loop_unroll_18(const T* first, int count, const std::string label) {
    int i;
    start_timer();
    for(i = 0; i < iterations; ++i) {
        T result = 0;
        int n = 0;
        for (; n < (count - (18-1)); n += 18) {
            {T temp = first[ n + 0 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 1 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 2 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 3 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 4 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 5 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 6 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 7 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 8 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 9 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 10 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 11 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 12 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 13 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 14 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 15 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 16 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 17 ]; temp = complete_hash_func( temp ); result += temp;}
        }
        for (; n < count; ++n) {
            result += complete_hash_func( first[n] );
        }
        check_sum<T>(result,label);
    }
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

template < typename T >
void test_loop_unroll_19(const T* first, int count, const std::string label) {
    int i;
    start_timer();
    for(i = 0; i < iterations; ++i) {
        T result = 0;
        int n = 0;
        for (; n < (count - (19-1)); n += 19) {
            {T temp = first[ n + 0 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 1 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 2 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 3 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 4 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 5 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 6 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 7 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 8 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 9 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 10 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 11 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 12 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 13 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 14 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 15 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 16 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 17 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 18 ]; temp = complete_hash_func( temp ); result += temp;}
        }
        for (; n < count; ++n) {
            result += complete_hash_func( first[n] );
        }
        check_sum<T>(result,label);
    }
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

template < typename T >
void test_loop_unroll_20(const T* first, int count, const std::string label) {
    int i;
    start_timer();
    for(i = 0; i < iterations; ++i) {
        T result = 0;
        int n = 0;
        for (; n < (count - (20-1)); n += 20) {
            {T temp = first[ n + 0 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 1 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 2 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 3 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 4 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 5 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 6 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 7 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 8 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 9 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 10 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 11 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 12 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 13 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 14 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 15 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 16 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 17 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 18 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 19 ]; temp = complete_hash_func( temp ); result += temp;}
        }
        for (; n < count; ++n) {
            result += complete_hash_func( first[n] );
        }
        check_sum<T>(result,label);
    }
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

template < typename T >
void test_loop_unroll_21(const T* first, int count, const std::string label) {
    int i;
    start_timer();
    for(i = 0; i < iterations; ++i) {
        T result = 0;
        int n = 0;
        for (; n < (count - (21-1)); n += 21) {
            {T temp = first[ n + 0 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 1 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 2 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 3 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 4 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 5 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 6 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 7 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 8 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 9 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 10 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 11 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 12 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 13 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 14 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 15 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 16 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 17 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 18 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 19 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 20 ]; temp = complete_hash_func( temp ); result += temp;}
        }
        for (; n < count; ++n) {
            result += complete_hash_func( first[n] );
        }
        check_sum<T>(result,label);
    }
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

template < typename T >
void test_loop_unroll_22(const T* first, int count, const std::string label) {
    int i;
    start_timer();
    for(i = 0; i < iterations; ++i) {
        T result = 0;
        int n = 0;
        for (; n < (count - (22-1)); n += 22) {
            {T temp = first[ n + 0 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 1 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 2 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 3 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 4 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 5 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 6 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 7 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 8 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 9 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 10 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 11 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 12 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 13 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 14 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 15 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 16 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 17 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 18 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 19 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 20 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 21 ]; temp = complete_hash_func( temp ); result += temp;}
        }
        for (; n < count; ++n) {
            result += complete_hash_func( first[n] );
        }
        check_sum<T>(result,label);
    }
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

template < typename T >
void test_loop_unroll_23(const T* first, int count, const std::string label) {
    int i;
    start_timer();
    for(i = 0; i < iterations; ++i) {
        T result = 0;
        int n = 0;
        for (; n < (count - (23-1)); n += 23) {
            {T temp = first[ n + 0 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 1 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 2 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 3 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 4 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 5 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 6 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 7 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 8 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 9 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 10 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 11 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 12 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 13 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 14 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 15 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 16 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 17 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 18 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 19 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 20 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 21 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 22 ]; temp = complete_hash_func( temp ); result += temp;}
        }
        for (; n < count; ++n) {
            result += complete_hash_func( first[n] );
        }
        check_sum<T>(result,label);
    }
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

template < typename T >
void test_loop_unroll_24(const T* first, int count, const std::string label) {
    int i;
    start_timer();
    for(i = 0; i < iterations; ++i) {
        T result = 0;
        int n = 0;
        for (; n < (count - (24-1)); n += 24) {
            {T temp = first[ n + 0 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 1 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 2 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 3 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 4 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 5 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 6 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 7 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 8 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 9 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 10 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 11 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 12 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 13 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 14 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 15 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 16 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 17 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 18 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 19 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 20 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 21 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 22 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 23 ]; temp = complete_hash_func( temp ); result += temp;}
        }
        for (; n < count; ++n) {
            result += complete_hash_func( first[n] );
        }
        check_sum<T>(result,label);
    }
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

template < typename T >
void test_loop_unroll_25(const T* first, int count, const std::string label) {
    int i;
    start_timer();
    for(i = 0; i < iterations; ++i) {
        T result = 0;
        int n = 0;
        for (; n < (count - (25-1)); n += 25) {
            {T temp = first[ n + 0 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 1 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 2 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 3 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 4 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 5 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 6 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 7 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 8 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 9 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 10 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 11 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 12 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 13 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 14 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 15 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 16 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 17 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 18 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 19 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 20 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 21 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 22 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 23 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 24 ]; temp = complete_hash_func( temp ); result += temp;}
        }
        for (; n < count; ++n) {
            result += complete_hash_func( first[n] );
        }
        check_sum<T>(result,label);
    }
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

template < typename T >
void test_loop_unroll_26(const T* first, int count, const std::string label) {
    int i;
    start_timer();
    for(i = 0; i < iterations; ++i) {
        T result = 0;
        int n = 0;
        for (; n < (count - (26-1)); n += 26) {
            {T temp = first[ n + 0 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 1 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 2 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 3 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 4 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 5 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 6 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 7 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 8 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 9 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 10 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 11 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 12 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 13 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 14 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 15 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 16 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 17 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 18 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 19 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 20 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 21 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 22 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 23 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 24 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 25 ]; temp = complete_hash_func( temp ); result += temp;}
        }
        for (; n < count; ++n) {
            result += complete_hash_func( first[n] );
        }
        check_sum<T>(result,label);
    }
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

template < typename T >
void test_loop_unroll_27(const T* first, int count, const std::string label) {
    int i;
    start_timer();
    for(i = 0; i < iterations; ++i) {
        T result = 0;
        int n = 0;
        for (; n < (count - (27-1)); n += 27) {
            {T temp = first[ n + 0 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 1 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 2 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 3 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 4 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 5 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 6 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 7 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 8 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 9 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 10 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 11 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 12 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 13 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 14 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 15 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 16 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 17 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 18 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 19 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 20 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 21 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 22 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 23 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 24 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 25 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 26 ]; temp = complete_hash_func( temp ); result += temp;}
        }
        for (; n < count; ++n) {
            result += complete_hash_func( first[n] );
        }
        check_sum<T>(result,label);
    }
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

template < typename T >
void test_loop_unroll_28(const T* first, int count, const std::string label) {
    int i;
    start_timer();
    for(i = 0; i < iterations; ++i) {
        T result = 0;
        int n = 0;
        for (; n < (count - (28-1)); n += 28) {
            {T temp = first[ n + 0 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 1 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 2 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 3 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 4 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 5 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 6 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 7 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 8 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 9 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 10 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 11 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 12 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 13 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 14 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 15 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 16 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 17 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 18 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 19 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 20 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 21 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 22 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 23 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 24 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 25 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 26 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 27 ]; temp = complete_hash_func( temp ); result += temp;}
        }
        for (; n < count; ++n) {
            result += complete_hash_func( first[n] );
        }
        check_sum<T>(result,label);
    }
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

template < typename T >
void test_loop_unroll_29(const T* first, int count, const std::string label) {
    int i;
    start_timer();
    for(i = 0; i < iterations; ++i) {
        T result = 0;
        int n = 0;
        for (; n < (count - (29-1)); n += 29) {
            {T temp = first[ n + 0 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 1 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 2 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 3 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 4 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 5 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 6 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 7 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 8 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 9 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 10 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 11 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 12 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 13 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 14 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 15 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 16 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 17 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 18 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 19 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 20 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 21 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 22 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 23 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 24 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 25 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 26 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 27 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 28 ]; temp = complete_hash_func( temp ); result += temp;}
        }
        for (; n < count; ++n) {
            result += complete_hash_func( first[n] );
        }
        check_sum<T>(result,label);
    }
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

template < typename T >
void test_loop_unroll_30(const T* first, int count, const std::string label) {
    int i;
    start_timer();
    for(i = 0; i < iterations; ++i) {
        T result = 0;
        int n = 0;
        for (; n < (count - (30-1)); n += 30) {
            {T temp = first[ n + 0 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 1 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 2 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 3 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 4 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 5 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 6 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 7 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 8 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 9 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 10 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 11 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 12 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 13 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 14 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 15 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 16 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 17 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 18 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 19 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 20 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 21 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 22 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 23 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 24 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 25 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 26 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 27 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 28 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 29 ]; temp = complete_hash_func( temp ); result += temp;}
        }
        for (; n < count; ++n) {
            result += complete_hash_func( first[n] );
        }
        check_sum<T>(result,label);
    }
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

template < typename T >
void test_loop_unroll_31(const T* first, int count, const std::string label) {
    int i;
    start_timer();
    for(i = 0; i < iterations; ++i) {
        T result = 0;
        int n = 0;
        for (; n < (count - (31-1)); n += 31) {
            {T temp = first[ n + 0 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 1 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 2 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 3 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 4 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 5 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 6 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 7 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 8 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 9 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 10 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 11 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 12 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 13 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 14 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 15 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 16 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 17 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 18 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 19 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 20 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 21 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 22 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 23 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 24 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 25 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 26 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 27 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 28 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 29 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 30 ]; temp = complete_hash_func( temp ); result += temp;}
        }
        for (; n < count; ++n) {
            result += complete_hash_func( first[n] );
        }
        check_sum<T>(result,label);
    }
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

template < typename T >
void test_loop_unroll_32(const T* first, int count, const std::string label) {
    int i;
    start_timer();
    for(i = 0; i < iterations; ++i) {
        T result = 0;
        int n = 0;
        for (; n < (count - (32-1)); n += 32) {
            {T temp = first[ n + 0 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 1 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 2 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 3 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 4 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 5 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 6 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 7 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 8 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 9 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 10 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 11 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 12 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 13 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 14 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 15 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 16 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 17 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 18 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 19 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 20 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 21 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 22 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 23 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 24 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 25 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 26 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 27 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 28 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 29 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 30 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 31 ]; temp = complete_hash_func( temp ); result += temp;}
        }
        for (; n < count; ++n) {
            result += complete_hash_func( first[n] );
        }
        check_sum<T>(result,label);
    }
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

template < typename T >
void test_loop_unroll_33(const T* first, int count, const std::string label) {
    int i;
    start_timer();
    for(i = 0; i < iterations; ++i) {
        T result = 0;
        int n = 0;
        for (; n < (count - (33-1)); n += 33) {
            {T temp = first[ n + 0 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 1 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 2 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 3 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 4 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 5 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 6 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 7 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 8 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 9 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 10 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 11 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 12 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 13 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 14 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 15 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 16 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 17 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 18 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 19 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 20 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 21 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 22 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 23 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 24 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 25 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 26 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 27 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 28 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 29 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 30 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 31 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 32 ]; temp = complete_hash_func( temp ); result += temp;}
        }
        for (; n < count; ++n) {
            result += complete_hash_func( first[n] );
        }
        check_sum<T>(result,label);
    }
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

template < typename T >
void test_loop_unroll_34(const T* first, int count, const std::string label) {
    int i;
    start_timer();
    for(i = 0; i < iterations; ++i) {
        T result = 0;
        int n = 0;
        for (; n < (count - (34-1)); n += 34) {
            {T temp = first[ n + 0 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 1 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 2 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 3 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 4 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 5 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 6 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 7 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 8 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 9 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 10 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 11 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 12 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 13 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 14 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 15 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 16 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 17 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 18 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 19 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 20 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 21 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 22 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 23 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 24 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 25 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 26 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 27 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 28 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 29 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 30 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 31 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 32 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 33 ]; temp = complete_hash_func( temp ); result += temp;}
        }
        for (; n < count; ++n) {
            result += complete_hash_func( first[n] );
        }
        check_sum<T>(result,label);
    }
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

template < typename T >
void test_loop_unroll_35(const T* first, int count, const std::string label) {
    int i;
    start_timer();
    for(i = 0; i < iterations; ++i) {
        T result = 0;
        int n = 0;
        for (; n < (count - (35-1)); n += 35) {
            {T temp = first[ n + 0 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 1 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 2 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 3 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 4 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 5 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 6 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 7 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 8 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 9 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 10 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 11 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 12 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 13 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 14 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 15 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 16 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 17 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 18 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 19 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 20 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 21 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 22 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 23 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 24 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 25 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 26 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 27 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 28 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 29 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 30 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 31 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 32 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 33 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 34 ]; temp = complete_hash_func( temp ); result += temp;}
        }
        for (; n < count; ++n) {
            result += complete_hash_func( first[n] );
        }
        check_sum<T>(result,label);
    }
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

template < typename T >
void test_loop_unroll_36(const T* first, int count, const std::string label) {
    int i;
    start_timer();
    for(i = 0; i < iterations; ++i) {
        T result = 0;
        int n = 0;
        for (; n < (count - (36-1)); n += 36) {
            {T temp = first[ n + 0 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 1 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 2 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 3 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 4 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 5 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 6 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 7 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 8 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 9 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 10 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 11 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 12 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 13 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 14 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 15 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 16 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 17 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 18 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 19 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 20 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 21 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 22 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 23 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 24 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 25 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 26 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 27 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 28 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 29 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 30 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 31 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 32 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 33 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 34 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 35 ]; temp = complete_hash_func( temp ); result += temp;}
        }
        for (; n < count; ++n) {
            result += complete_hash_func( first[n] );
        }
        check_sum<T>(result,label);
    }
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

template < typename T >
void test_loop_unroll_37(const T* first, int count, const std::string label) {
    int i;
    start_timer();
    for(i = 0; i < iterations; ++i) {
        T result = 0;
        int n = 0;
        for (; n < (count - (37-1)); n += 37) {
            {T temp = first[ n + 0 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 1 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 2 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 3 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 4 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 5 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 6 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 7 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 8 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 9 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 10 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 11 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 12 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 13 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 14 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 15 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 16 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 17 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 18 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 19 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 20 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 21 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 22 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 23 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 24 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 25 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 26 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 27 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 28 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 29 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 30 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 31 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 32 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 33 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 34 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 35 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 36 ]; temp = complete_hash_func( temp ); result += temp;}
        }
        for (; n < count; ++n) {
            result += complete_hash_func( first[n] );
        }
        check_sum<T>(result,label);
    }
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

template < typename T >
void test_loop_unroll_38(const T* first, int count, const std::string label) {
    int i;
    start_timer();
    for(i = 0; i < iterations; ++i) {
        T result = 0;
        int n = 0;
        for (; n < (count - (38-1)); n += 38) {
            {T temp = first[ n + 0 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 1 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 2 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 3 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 4 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 5 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 6 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 7 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 8 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 9 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 10 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 11 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 12 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 13 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 14 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 15 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 16 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 17 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 18 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 19 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 20 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 21 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 22 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 23 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 24 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 25 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 26 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 27 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 28 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 29 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 30 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 31 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 32 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 33 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 34 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 35 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 36 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 37 ]; temp = complete_hash_func( temp ); result += temp;}
        }
        for (; n < count; ++n) {
            result += complete_hash_func( first[n] );
        }
        check_sum<T>(result,label);
    }
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

template < typename T >
void test_loop_unroll_39(const T* first, int count, const std::string label) {
    int i;
    start_timer();
    for(i = 0; i < iterations; ++i) {
        T result = 0;
        int n = 0;
        for (; n < (count - (39-1)); n += 39) {
            {T temp = first[ n + 0 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 1 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 2 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 3 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 4 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 5 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 6 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 7 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 8 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 9 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 10 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 11 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 12 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 13 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 14 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 15 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 16 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 17 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 18 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 19 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 20 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 21 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 22 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 23 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 24 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 25 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 26 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 27 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 28 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 29 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 30 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 31 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 32 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 33 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 34 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 35 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 36 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 37 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 38 ]; temp = complete_hash_func( temp ); result += temp;}
        }
        for (; n < count; ++n) {
            result += complete_hash_func( first[n] );
        }
        check_sum<T>(result,label);
    }
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

template < typename T >
void test_loop_unroll_40(const T* first, int count, const std::string label) {
    int i;
    start_timer();
    for(i = 0; i < iterations; ++i) {
        T result = 0;
        int n = 0;
        for (; n < (count - (40-1)); n += 40) {
            {T temp = first[ n + 0 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 1 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 2 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 3 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 4 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 5 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 6 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 7 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 8 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 9 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 10 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 11 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 12 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 13 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 14 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 15 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 16 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 17 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 18 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 19 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 20 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 21 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 22 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 23 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 24 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 25 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 26 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 27 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 28 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 29 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 30 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 31 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 32 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 33 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 34 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 35 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 36 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 37 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 38 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 39 ]; temp = complete_hash_func( temp ); result += temp;}
        }
        for (; n < count; ++n) {
            result += complete_hash_func( first[n] );
        }
        check_sum<T>(result,label);
    }
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

template < typename T >
void test_loop_unroll_41(const T* first, int count, const std::string label) {
    int i;
    start_timer();
    for(i = 0; i < iterations; ++i) {
        T result = 0;
        int n = 0;
        for (; n < (count - (41-1)); n += 41) {
            {T temp = first[ n + 0 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 1 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 2 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 3 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 4 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 5 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 6 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 7 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 8 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 9 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 10 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 11 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 12 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 13 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 14 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 15 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 16 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 17 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 18 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 19 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 20 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 21 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 22 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 23 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 24 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 25 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 26 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 27 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 28 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 29 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 30 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 31 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 32 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 33 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 34 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 35 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 36 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 37 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 38 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 39 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 40 ]; temp = complete_hash_func( temp ); result += temp;}
        }
        for (; n < count; ++n) {
            result += complete_hash_func( first[n] );
        }
        check_sum<T>(result,label);
    }
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

template < typename T >
void test_loop_unroll_42(const T* first, int count, const std::string label) {
    int i;
    start_timer();
    for(i = 0; i < iterations; ++i) {
        T result = 0;
        int n = 0;
        for (; n < (count - (42-1)); n += 42) {
            {T temp = first[ n + 0 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 1 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 2 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 3 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 4 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 5 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 6 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 7 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 8 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 9 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 10 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 11 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 12 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 13 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 14 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 15 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 16 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 17 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 18 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 19 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 20 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 21 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 22 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 23 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 24 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 25 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 26 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 27 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 28 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 29 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 30 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 31 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 32 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 33 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 34 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 35 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 36 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 37 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 38 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 39 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 40 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 41 ]; temp = complete_hash_func( temp ); result += temp;}
        }
        for (; n < count; ++n) {
            result += complete_hash_func( first[n] );
        }
        check_sum<T>(result,label);
    }
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

template < typename T >
void test_loop_unroll_43(const T* first, int count, const std::string label) {
    int i;
    start_timer();
    for(i = 0; i < iterations; ++i) {
        T result = 0;
        int n = 0;
        for (; n < (count - (43-1)); n += 43) {
            {T temp = first[ n + 0 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 1 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 2 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 3 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 4 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 5 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 6 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 7 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 8 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 9 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 10 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 11 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 12 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 13 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 14 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 15 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 16 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 17 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 18 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 19 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 20 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 21 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 22 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 23 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 24 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 25 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 26 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 27 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 28 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 29 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 30 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 31 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 32 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 33 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 34 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 35 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 36 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 37 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 38 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 39 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 40 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 41 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 42 ]; temp = complete_hash_func( temp ); result += temp;}
        }
        for (; n < count; ++n) {
            result += complete_hash_func( first[n] );
        }
        check_sum<T>(result,label);
    }
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

template < typename T >
void test_loop_unroll_44(const T* first, int count, const std::string label) {
    int i;
    start_timer();
    for(i = 0; i < iterations; ++i) {
        T result = 0;
        int n = 0;
        for (; n < (count - (44-1)); n += 44) {
            {T temp = first[ n + 0 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 1 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 2 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 3 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 4 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 5 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 6 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 7 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 8 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 9 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 10 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 11 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 12 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 13 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 14 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 15 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 16 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 17 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 18 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 19 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 20 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 21 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 22 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 23 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 24 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 25 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 26 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 27 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 28 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 29 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 30 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 31 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 32 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 33 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 34 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 35 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 36 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 37 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 38 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 39 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 40 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 41 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 42 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 43 ]; temp = complete_hash_func( temp ); result += temp;}
        }
        for (; n < count; ++n) {
            result += complete_hash_func( first[n] );
        }
        check_sum<T>(result,label);
    }
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

template < typename T >
void test_loop_unroll_45(const T* first, int count, const std::string label) {
    int i;
    start_timer();
    for(i = 0; i < iterations; ++i) {
        T result = 0;
        int n = 0;
        for (; n < (count - (45-1)); n += 45) {
            {T temp = first[ n + 0 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 1 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 2 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 3 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 4 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 5 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 6 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 7 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 8 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 9 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 10 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 11 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 12 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 13 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 14 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 15 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 16 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 17 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 18 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 19 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 20 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 21 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 22 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 23 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 24 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 25 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 26 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 27 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 28 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 29 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 30 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 31 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 32 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 33 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 34 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 35 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 36 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 37 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 38 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 39 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 40 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 41 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 42 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 43 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 44 ]; temp = complete_hash_func( temp ); result += temp;}
        }
        for (; n < count; ++n) {
            result += complete_hash_func( first[n] );
        }
        check_sum<T>(result,label);
    }
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

template < typename T >
void test_loop_unroll_46(const T* first, int count, const std::string label) {
    int i;
    start_timer();
    for(i = 0; i < iterations; ++i) {
        T result = 0;
        int n = 0;
        for (; n < (count - (46-1)); n += 46) {
            {T temp = first[ n + 0 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 1 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 2 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 3 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 4 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 5 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 6 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 7 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 8 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 9 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 10 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 11 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 12 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 13 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 14 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 15 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 16 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 17 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 18 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 19 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 20 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 21 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 22 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 23 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 24 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 25 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 26 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 27 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 28 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 29 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 30 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 31 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 32 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 33 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 34 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 35 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 36 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 37 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 38 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 39 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 40 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 41 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 42 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 43 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 44 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 45 ]; temp = complete_hash_func( temp ); result += temp;}
        }
        for (; n < count; ++n) {
            result += complete_hash_func( first[n] );
        }
        check_sum<T>(result,label);
    }
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

template < typename T >
void test_loop_unroll_47(const T* first, int count, const std::string label) {
    int i;
    start_timer();
    for(i = 0; i < iterations; ++i) {
        T result = 0;
        int n = 0;
        for (; n < (count - (47-1)); n += 47) {
            {T temp = first[ n + 0 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 1 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 2 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 3 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 4 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 5 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 6 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 7 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 8 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 9 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 10 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 11 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 12 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 13 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 14 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 15 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 16 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 17 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 18 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 19 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 20 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 21 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 22 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 23 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 24 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 25 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 26 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 27 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 28 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 29 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 30 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 31 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 32 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 33 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 34 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 35 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 36 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 37 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 38 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 39 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 40 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 41 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 42 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 43 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 44 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 45 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 46 ]; temp = complete_hash_func( temp ); result += temp;}
        }
        for (; n < count; ++n) {
            result += complete_hash_func( first[n] );
        }
        check_sum<T>(result,label);
    }
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

template < typename T >
void test_loop_unroll_48(const T* first, int count, const std::string label) {
    int i;
    start_timer();
    for(i = 0; i < iterations; ++i) {
        T result = 0;
        int n = 0;
        for (; n < (count - (48-1)); n += 48) {
            {T temp = first[ n + 0 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 1 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 2 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 3 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 4 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 5 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 6 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 7 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 8 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 9 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 10 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 11 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 12 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 13 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 14 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 15 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 16 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 17 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 18 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 19 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 20 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 21 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 22 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 23 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 24 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 25 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 26 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 27 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 28 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 29 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 30 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 31 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 32 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 33 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 34 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 35 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 36 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 37 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 38 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 39 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 40 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 41 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 42 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 43 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 44 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 45 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 46 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 47 ]; temp = complete_hash_func( temp ); result += temp;}
        }
        for (; n < count; ++n) {
            result += complete_hash_func( first[n] );
        }
        check_sum<T>(result,label);
    }
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

template < typename T >
void test_loop_unroll_49(const T* first, int count, const std::string label) {
    int i;
    start_timer();
    for(i = 0; i < iterations; ++i) {
        T result = 0;
        int n = 0;
        for (; n < (count - (49-1)); n += 49) {
            {T temp = first[ n + 0 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 1 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 2 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 3 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 4 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 5 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 6 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 7 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 8 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 9 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 10 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 11 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 12 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 13 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 14 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 15 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 16 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 17 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 18 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 19 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 20 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 21 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 22 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 23 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 24 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 25 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 26 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 27 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 28 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 29 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 30 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 31 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 32 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 33 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 34 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 35 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 36 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 37 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 38 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 39 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 40 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 41 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 42 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 43 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 44 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 45 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 46 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 47 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 48 ]; temp = complete_hash_func( temp ); result += temp;}
        }
        for (; n < count; ++n) {
            result += complete_hash_func( first[n] );
        }
        check_sum<T>(result,label);
    }
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

template < typename T >
void test_loop_unroll_50(const T* first, int count, const std::string label) {
    int i;
    start_timer();
    for(i = 0; i < iterations; ++i) {
        T result = 0;
        int n = 0;
        for (; n < (count - (50-1)); n += 50) {
            {T temp = first[ n + 0 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 1 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 2 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 3 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 4 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 5 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 6 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 7 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 8 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 9 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 10 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 11 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 12 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 13 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 14 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 15 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 16 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 17 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 18 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 19 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 20 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 21 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 22 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 23 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 24 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 25 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 26 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 27 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 28 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 29 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 30 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 31 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 32 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 33 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 34 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 35 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 36 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 37 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 38 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 39 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 40 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 41 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 42 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 43 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 44 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 45 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 46 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 47 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 48 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 49 ]; temp = complete_hash_func( temp ); result += temp;}
        }
        for (; n < count; ++n) {
            result += complete_hash_func( first[n] );
        }
        check_sum<T>(result,label);
    }
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

template < typename T >
void test_loop_unroll_51(const T* first, int count, const std::string label) {
    int i;
    start_timer();
    for(i = 0; i < iterations; ++i) {
        T result = 0;
        int n = 0;
        for (; n < (count - (51-1)); n += 51) {
            {T temp = first[ n + 0 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 1 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 2 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 3 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 4 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 5 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 6 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 7 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 8 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 9 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 10 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 11 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 12 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 13 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 14 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 15 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 16 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 17 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 18 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 19 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 20 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 21 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 22 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 23 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 24 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 25 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 26 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 27 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 28 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 29 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 30 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 31 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 32 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 33 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 34 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 35 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 36 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 37 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 38 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 39 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 40 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 41 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 42 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 43 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 44 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 45 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 46 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 47 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 48 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 49 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 50 ]; temp = complete_hash_func( temp ); result += temp;}
        }
        for (; n < count; ++n) {
            result += complete_hash_func( first[n] );
        }
        check_sum<T>(result,label);
    }
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

template < typename T >
void test_loop_unroll_52(const T* first, int count, const std::string label) {
    int i;
    start_timer();
    for(i = 0; i < iterations; ++i) {
        T result = 0;
        int n = 0;
        for (; n < (count - (52-1)); n += 52) {
            {T temp = first[ n + 0 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 1 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 2 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 3 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 4 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 5 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 6 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 7 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 8 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 9 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 10 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 11 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 12 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 13 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 14 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 15 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 16 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 17 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 18 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 19 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 20 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 21 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 22 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 23 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 24 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 25 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 26 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 27 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 28 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 29 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 30 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 31 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 32 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 33 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 34 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 35 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 36 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 37 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 38 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 39 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 40 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 41 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 42 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 43 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 44 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 45 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 46 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 47 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 48 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 49 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 50 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 51 ]; temp = complete_hash_func( temp ); result += temp;}
        }
        for (; n < count; ++n) {
            result += complete_hash_func( first[n] );
        }
        check_sum<T>(result,label);
    }
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

template < typename T >
void test_loop_unroll_53(const T* first, int count, const std::string label) {
    int i;
    start_timer();
    for(i = 0; i < iterations; ++i) {
        T result = 0;
        int n = 0;
        for (; n < (count - (53-1)); n += 53) {
            {T temp = first[ n + 0 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 1 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 2 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 3 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 4 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 5 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 6 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 7 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 8 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 9 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 10 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 11 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 12 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 13 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 14 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 15 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 16 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 17 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 18 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 19 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 20 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 21 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 22 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 23 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 24 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 25 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 26 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 27 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 28 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 29 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 30 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 31 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 32 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 33 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 34 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 35 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 36 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 37 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 38 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 39 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 40 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 41 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 42 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 43 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 44 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 45 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 46 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 47 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 48 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 49 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 50 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 51 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 52 ]; temp = complete_hash_func( temp ); result += temp;}
        }
        for (; n < count; ++n) {
            result += complete_hash_func( first[n] );
        }
        check_sum<T>(result,label);
    }
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

template < typename T >
void test_loop_unroll_54(const T* first, int count, const std::string label) {
    int i;
    start_timer();
    for(i = 0; i < iterations; ++i) {
        T result = 0;
        int n = 0;
        for (; n < (count - (54-1)); n += 54) {
            {T temp = first[ n + 0 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 1 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 2 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 3 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 4 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 5 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 6 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 7 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 8 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 9 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 10 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 11 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 12 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 13 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 14 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 15 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 16 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 17 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 18 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 19 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 20 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 21 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 22 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 23 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 24 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 25 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 26 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 27 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 28 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 29 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 30 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 31 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 32 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 33 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 34 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 35 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 36 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 37 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 38 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 39 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 40 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 41 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 42 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 43 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 44 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 45 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 46 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 47 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 48 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 49 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 50 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 51 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 52 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 53 ]; temp = complete_hash_func( temp ); result += temp;}
        }
        for (; n < count; ++n) {
            result += complete_hash_func( first[n] );
        }
        check_sum<T>(result,label);
    }
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

template < typename T >
void test_loop_unroll_55(const T* first, int count, const std::string label) {
    int i;
    start_timer();
    for(i = 0; i < iterations; ++i) {
        T result = 0;
        int n = 0;
        for (; n < (count - (55-1)); n += 55) {
            {T temp = first[ n + 0 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 1 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 2 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 3 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 4 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 5 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 6 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 7 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 8 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 9 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 10 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 11 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 12 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 13 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 14 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 15 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 16 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 17 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 18 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 19 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 20 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 21 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 22 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 23 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 24 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 25 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 26 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 27 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 28 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 29 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 30 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 31 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 32 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 33 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 34 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 35 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 36 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 37 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 38 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 39 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 40 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 41 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 42 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 43 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 44 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 45 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 46 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 47 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 48 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 49 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 50 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 51 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 52 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 53 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 54 ]; temp = complete_hash_func( temp ); result += temp;}
        }
        for (; n < count; ++n) {
            result += complete_hash_func( first[n] );
        }
        check_sum<T>(result,label);
    }
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

template < typename T >
void test_loop_unroll_56(const T* first, int count, const std::string label) {
    int i;
    start_timer();
    for(i = 0; i < iterations; ++i) {
        T result = 0;
        int n = 0;
        for (; n < (count - (56-1)); n += 56) {
            {T temp = first[ n + 0 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 1 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 2 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 3 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 4 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 5 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 6 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 7 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 8 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 9 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 10 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 11 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 12 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 13 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 14 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 15 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 16 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 17 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 18 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 19 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 20 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 21 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 22 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 23 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 24 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 25 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 26 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 27 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 28 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 29 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 30 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 31 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 32 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 33 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 34 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 35 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 36 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 37 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 38 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 39 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 40 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 41 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 42 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 43 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 44 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 45 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 46 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 47 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 48 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 49 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 50 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 51 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 52 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 53 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 54 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 55 ]; temp = complete_hash_func( temp ); result += temp;}
        }
        for (; n < count; ++n) {
            result += complete_hash_func( first[n] );
        }
        check_sum<T>(result,label);
    }
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

template < typename T >
void test_loop_unroll_57(const T* first, int count, const std::string label) {
    int i;
    start_timer();
    for(i = 0; i < iterations; ++i) {
        T result = 0;
        int n = 0;
        for (; n < (count - (57-1)); n += 57) {
            {T temp = first[ n + 0 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 1 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 2 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 3 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 4 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 5 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 6 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 7 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 8 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 9 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 10 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 11 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 12 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 13 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 14 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 15 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 16 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 17 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 18 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 19 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 20 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 21 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 22 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 23 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 24 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 25 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 26 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 27 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 28 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 29 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 30 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 31 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 32 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 33 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 34 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 35 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 36 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 37 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 38 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 39 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 40 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 41 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 42 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 43 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 44 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 45 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 46 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 47 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 48 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 49 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 50 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 51 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 52 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 53 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 54 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 55 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 56 ]; temp = complete_hash_func( temp ); result += temp;}
        }
        for (; n < count; ++n) {
            result += complete_hash_func( first[n] );
        }
        check_sum<T>(result,label);
    }
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

template < typename T >
void test_loop_unroll_58(const T* first, int count, const std::string label) {
    int i;
    start_timer();
    for(i = 0; i < iterations; ++i) {
        T result = 0;
        int n = 0;
        for (; n < (count - (58-1)); n += 58) {
            {T temp = first[ n + 0 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 1 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 2 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 3 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 4 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 5 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 6 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 7 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 8 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 9 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 10 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 11 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 12 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 13 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 14 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 15 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 16 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 17 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 18 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 19 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 20 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 21 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 22 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 23 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 24 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 25 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 26 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 27 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 28 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 29 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 30 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 31 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 32 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 33 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 34 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 35 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 36 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 37 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 38 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 39 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 40 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 41 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 42 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 43 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 44 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 45 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 46 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 47 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 48 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 49 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 50 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 51 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 52 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 53 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 54 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 55 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 56 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 57 ]; temp = complete_hash_func( temp ); result += temp;}
        }
        for (; n < count; ++n) {
            result += complete_hash_func( first[n] );
        }
        check_sum<T>(result,label);
    }
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

template < typename T >
void test_loop_unroll_59(const T* first, int count, const std::string label) {
    int i;
    start_timer();
    for(i = 0; i < iterations; ++i) {
        T result = 0;
        int n = 0;
        for (; n < (count - (59-1)); n += 59) {
            {T temp = first[ n + 0 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 1 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 2 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 3 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 4 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 5 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 6 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 7 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 8 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 9 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 10 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 11 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 12 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 13 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 14 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 15 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 16 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 17 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 18 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 19 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 20 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 21 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 22 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 23 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 24 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 25 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 26 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 27 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 28 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 29 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 30 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 31 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 32 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 33 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 34 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 35 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 36 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 37 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 38 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 39 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 40 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 41 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 42 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 43 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 44 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 45 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 46 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 47 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 48 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 49 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 50 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 51 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 52 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 53 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 54 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 55 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 56 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 57 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 58 ]; temp = complete_hash_func( temp ); result += temp;}
        }
        for (; n < count; ++n) {
            result += complete_hash_func( first[n] );
        }
        check_sum<T>(result,label);
    }
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

template < typename T >
void test_loop_unroll_60(const T* first, int count, const std::string label) {
    int i;
    start_timer();
    for(i = 0; i < iterations; ++i) {
        T result = 0;
        int n = 0;
        for (; n < (count - (60-1)); n += 60) {
            {T temp = first[ n + 0 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 1 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 2 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 3 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 4 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 5 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 6 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 7 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 8 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 9 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 10 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 11 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 12 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 13 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 14 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 15 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 16 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 17 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 18 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 19 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 20 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 21 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 22 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 23 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 24 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 25 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 26 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 27 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 28 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 29 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 30 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 31 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 32 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 33 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 34 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 35 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 36 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 37 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 38 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 39 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 40 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 41 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 42 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 43 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 44 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 45 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 46 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 47 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 48 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 49 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 50 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 51 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 52 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 53 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 54 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 55 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 56 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 57 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 58 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 59 ]; temp = complete_hash_func( temp ); result += temp;}
        }
        for (; n < count; ++n) {
            result += complete_hash_func( first[n] );
        }
        check_sum<T>(result,label);
    }
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

template < typename T >
void test_loop_unroll_61(const T* first, int count, const std::string label) {
    int i;
    start_timer();
    for(i = 0; i < iterations; ++i) {
        T result = 0;
        int n = 0;
        for (; n < (count - (61-1)); n += 61) {
            {T temp = first[ n + 0 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 1 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 2 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 3 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 4 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 5 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 6 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 7 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 8 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 9 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 10 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 11 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 12 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 13 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 14 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 15 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 16 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 17 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 18 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 19 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 20 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 21 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 22 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 23 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 24 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 25 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 26 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 27 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 28 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 29 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 30 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 31 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 32 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 33 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 34 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 35 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 36 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 37 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 38 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 39 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 40 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 41 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 42 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 43 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 44 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 45 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 46 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 47 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 48 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 49 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 50 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 51 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 52 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 53 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 54 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 55 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 56 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 57 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 58 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 59 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 60 ]; temp = complete_hash_func( temp ); result += temp;}
        }
        for (; n < count; ++n) {
            result += complete_hash_func( first[n] );
        }
        check_sum<T>(result,label);
    }
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

template < typename T >
void test_loop_unroll_62(const T* first, int count, const std::string label) {
    int i;
    start_timer();
    for(i = 0; i < iterations; ++i) {
        T result = 0;
        int n = 0;
        for (; n < (count - (62-1)); n += 62) {
            {T temp = first[ n + 0 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 1 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 2 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 3 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 4 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 5 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 6 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 7 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 8 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 9 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 10 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 11 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 12 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 13 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 14 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 15 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 16 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 17 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 18 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 19 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 20 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 21 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 22 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 23 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 24 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 25 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 26 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 27 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 28 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 29 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 30 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 31 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 32 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 33 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 34 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 35 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 36 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 37 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 38 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 39 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 40 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 41 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 42 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 43 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 44 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 45 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 46 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 47 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 48 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 49 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 50 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 51 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 52 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 53 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 54 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 55 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 56 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 57 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 58 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 59 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 60 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 61 ]; temp = complete_hash_func( temp ); result += temp;}
        }
        for (; n < count; ++n) {
            result += complete_hash_func( first[n] );
        }
        check_sum<T>(result,label);
    }
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

template < typename T >
void test_loop_unroll_63(const T* first, int count, const std::string label) {
    int i;
    start_timer();
    for(i = 0; i < iterations; ++i) {
        T result = 0;
        int n = 0;
        for (; n < (count - (63-1)); n += 63) {
            {T temp = first[ n + 0 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 1 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 2 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 3 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 4 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 5 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 6 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 7 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 8 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 9 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 10 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 11 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 12 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 13 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 14 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 15 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 16 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 17 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 18 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 19 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 20 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 21 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 22 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 23 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 24 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 25 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 26 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 27 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 28 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 29 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 30 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 31 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 32 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 33 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 34 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 35 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 36 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 37 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 38 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 39 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 40 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 41 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 42 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 43 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 44 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 45 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 46 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 47 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 48 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 49 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 50 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 51 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 52 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 53 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 54 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 55 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 56 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 57 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 58 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 59 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 60 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 61 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 62 ]; temp = complete_hash_func( temp ); result += temp;}
        }
        for (; n < count; ++n) {
            result += complete_hash_func( first[n] );
        }
        check_sum<T>(result,label);
    }
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

template < typename T >
void test_loop_unroll_64(const T* first, int count, const std::string label) {
    int i;
    start_timer();
    for(i = 0; i < iterations; ++i) {
        T result = 0;
        int n = 0;
        for (; n < (count - (64-1)); n += 64) {
            {T temp = first[ n + 0 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 1 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 2 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 3 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 4 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 5 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 6 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 7 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 8 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 9 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 10 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 11 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 12 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 13 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 14 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 15 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 16 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 17 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 18 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 19 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 20 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 21 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 22 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 23 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 24 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 25 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 26 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 27 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 28 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 29 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 30 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 31 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 32 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 33 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 34 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 35 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 36 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 37 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 38 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 39 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 40 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 41 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 42 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 43 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 44 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 45 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 46 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 47 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 48 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 49 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 50 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 51 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 52 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 53 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 54 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 55 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 56 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 57 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 58 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 59 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 60 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 61 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 62 ]; temp = complete_hash_func( temp ); result += temp;}
            {T temp = first[ n + 63 ]; temp = complete_hash_func( temp ); result += temp;}
        }
        for (; n < count; ++n) {
            result += complete_hash_func( first[n] );
        }
        check_sum<T>(result,label);
    }
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

template < typename T >
void test_manual_loops(const T* data, int count, const std::string label) {
    test_loop_unroll_1<T>( data, count, label + " 1" );
    test_loop_unroll_2<T>( data, count, label + " 2" );
    test_loop_unroll_3<T>( data, count, label + " 3" );
    test_loop_unroll_4<T>( data, count, label + " 4" );
    test_loop_unroll_5<T>( data, count, label + " 5" );
    test_loop_unroll_6<T>( data, count, label + " 6" );
    test_loop_unroll_7<T>( data, count, label + " 7" );
    test_loop_unroll_8<T>( data, count, label + " 8" );
    test_loop_unroll_9<T>( data, count, label + " 9" );
    test_loop_unroll_10<T>( data, count, label + " 10" );
    test_loop_unroll_11<T>( data, count, label + " 11" );
    test_loop_unroll_12<T>( data, count, label + " 12" );
    test_loop_unroll_13<T>( data, count, label + " 13" );
    test_loop_unroll_14<T>( data, count, label + " 14" );
    test_loop_unroll_15<T>( data, count, label + " 15" );
    test_loop_unroll_16<T>( data, count, label + " 16" );
    test_loop_unroll_17<T>( data, count, label + " 17" );
    test_loop_unroll_18<T>( data, count, label + " 18" );
    test_loop_unroll_19<T>( data, count, label + " 19" );
    test_loop_unroll_20<T>( data, count, label + " 20" );
    test_loop_unroll_21<T>( data, count, label + " 21" );
    test_loop_unroll_22<T>( data, count, label + " 22" );
    test_loop_unroll_23<T>( data, count, label + " 23" );
    test_loop_unroll_24<T>( data, count, label + " 24" );
    test_loop_unroll_25<T>( data, count, label + " 25" );
    test_loop_unroll_26<T>( data, count, label + " 26" );
    test_loop_unroll_27<T>( data, count, label + " 27" );
    test_loop_unroll_28<T>( data, count, label + " 28" );
    test_loop_unroll_29<T>( data, count, label + " 29" );
    test_loop_unroll_30<T>( data, count, label + " 30" );
    test_loop_unroll_31<T>( data, count, label + " 31" );
    test_loop_unroll_32<T>( data, count, label + " 32" );
    test_loop_unroll_33<T>( data, count, label + " 33" );
    test_loop_unroll_34<T>( data, count, label + " 34" );
    test_loop_unroll_35<T>( data, count, label + " 35" );
    test_loop_unroll_36<T>( data, count, label + " 36" );
    test_loop_unroll_37<T>( data, count, label + " 37" );
    test_loop_unroll_38<T>( data, count, label + " 38" );
    test_loop_unroll_39<T>( data, count, label + " 39" );
    test_loop_unroll_40<T>( data, count, label + " 40" );
    test_loop_unroll_41<T>( data, count, label + " 41" );
    test_loop_unroll_42<T>( data, count, label + " 42" );
    test_loop_unroll_43<T>( data, count, label + " 43" );
    test_loop_unroll_44<T>( data, count, label + " 44" );
    test_loop_unroll_45<T>( data, count, label + " 45" );
    test_loop_unroll_46<T>( data, count, label + " 46" );
    test_loop_unroll_47<T>( data, count, label + " 47" );
    test_loop_unroll_48<T>( data, count, label + " 48" );
    test_loop_unroll_49<T>( data, count, label + " 49" );
    test_loop_unroll_50<T>( data, count, label + " 50" );
    test_loop_unroll_51<T>( data, count, label + " 51" );
    test_loop_unroll_52<T>( data, count, label + " 52" );
    test_loop_unroll_53<T>( data, count, label + " 53" );
    test_loop_unroll_54<T>( data, count, label + " 54" );
    test_loop_unroll_55<T>( data, count, label + " 55" );
    test_loop_unroll_56<T>( data, count, label + " 56" );
    test_loop_unroll_57<T>( data, count, label + " 57" );
    test_loop_unroll_58<T>( data, count, label + " 58" );
    test_loop_unroll_59<T>( data, count, label + " 59" );
    test_loop_unroll_60<T>( data, count, label + " 60" );
    test_loop_unroll_61<T>( data, count, label + " 61" );
    test_loop_unroll_62<T>( data, count, label + " 62" );
    test_loop_unroll_63<T>( data, count, label + " 63" );
    test_loop_unroll_64<T>( data, count, label + " 64" );
    std::string temp = label;
    summarize(temp.c_str(), SIZE, iterations, kDontShowGMeans, kDontShowPenalty );
}

/******************************************************************************/

static
void print_manual_tests( size_t MaxUnroll ) {

    // each unrolled template
    for (size_t N = 1; N <= MaxUnroll; ++N ) {
        printf("template < typename T >\n");
        printf("void test_loop_unroll_%zd(const T* first, int count, const std::string label) {\n", N );
            printf("int i;\n");
            printf("start_timer();\n");
        
            printf("for(i = 0; i < iterations; ++i) {\n");
                printf("T result = 0;\n");
                printf("int n = 0;\n");
        
                printf("for (; n < (count - (%zd-1)); n += %zd) {\n", N, N );
                    for (size_t k = 0; k < N; ++k) {
                        printf("{T temp = first[ n + %zd ]; temp = complete_hash_func( temp ); result += temp;}\n", k );
                    }
                printf("}\n");
        
                printf("for (; n < count; ++n) {\n");
                    printf("result += complete_hash_func( first[n] );\n");
                printf("}\n");
        
                printf("check_sum<T>(result,label);\n");
            printf("}\n");
        
            printf("gLabels.push_back( label );\n");
            printf("record_result( timer(), gLabels.back().c_str() );\n");
        printf("}\n\n");
    }
    
    // test function that calls all of them
    printf("template < typename T >\n");
    printf("void test_manual_loops(const T* data, int count, const std::string label) {\n" );
    for (size_t N = 1; N <= MaxUnroll; ++N ) {
        printf("test_loop_unroll_%zd<T>( data, count, label + \" %zd\" );\n", N, N );
    }
    printf("std::string temp = label;\n");
    printf("summarize(temp.c_str(), SIZE, iterations, kDontShowGMeans, kDontShowPenalty );\n");
    printf("}\n\n");
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


    test_manual_loops<T>( data, SIZE, myTypeName + " template unroll manual" );
    
    loop_tests_linear< kUnrollLimit, T >::do_test( data, myTypeName + " template unroll linear" );
    std::string temp1 = myTypeName + " template unrolling linear";
    summarize(temp1.c_str(), SIZE, iterations, kDontShowGMeans, kDontShowPenalty );
    
    loop_tests_binary< kUnrollLimit, T >::do_test( data, myTypeName + " template unroll binary" );
    std::string temp2 = myTypeName + " template unrolling binary";
    summarize(temp2.c_str(), SIZE, iterations, kDontShowGMeans, kDontShowPenalty );
    
    loop_tests_trinary< kUnrollLimit, T >::do_test( data, myTypeName + " template unroll trinary" );
    std::string temp3 = myTypeName + " template unrolling trinary";
    summarize(temp3.c_str(), SIZE, iterations, kDontShowGMeans, kDontShowPenalty );
    
    loop_tests_quaternary< kUnrollLimit, T >::do_test( data, myTypeName + " template unroll quaternary" );
    std::string temp4 = myTypeName + " template unrolling quaternary";
    summarize(temp4.c_str(), SIZE, iterations, kDontShowGMeans, kDontShowPenalty );
    
    loop_tests_octal< kUnrollLimit, T >::do_test( data, myTypeName + " template unroll octal" );
    std::string temp5 = myTypeName + " template unrolling octal";
    summarize(temp5.c_str(), SIZE, iterations, kDontShowGMeans, kDontShowPenalty );
    
    loop_tests_decimal< kUnrollLimit, T >::do_test( data, myTypeName + " template unroll decimal" );
    std::string temp6 = myTypeName + " template unrolling decimal";
    summarize(temp6.c_str(), SIZE, iterations, kDontShowGMeans, kDontShowPenalty );

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
    
    
    TestOneType<int32_t>();
    
    iterations /= 2;
    TestOneType<double>();


#if 0
// this only needs to be used when changing the manual unroll code
    print_manual_tests( kUnrollLimit );
#endif
    
    return 0;
}

// the end
/******************************************************************************/
/******************************************************************************/
