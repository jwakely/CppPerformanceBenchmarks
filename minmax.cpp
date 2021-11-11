/*
    Copyright 2008-2009 Adobe Systems Incorporated
    Copyright 2018-2019 Chris Cox
    Distributed under the MIT License (see accompanying file LICENSE_1_0_0.txt
    or a copy at http://stlab.adobe.com/licenses.html )


Goal: Test performance of various minimum, maximum, and pin/clamp idioms.

Assumptions:
    1) The compiler will optimize minimum and maximum operations.
    
    2) The compiler should recognize ineffecient minimum or maximum idioms
        and substitute efficient methods.


See also: https://graphics.stanford.edu/~seander/bithacks.html#IntegerMinOrMax

NOTE - removed the unsafe branchless calculations (hence some odd numbering)




TODO - C++ 17 std::clamp
        NOT yet safe for several compilers

*/

/******************************************************************************/

#include "benchmark_stdint.hpp"
#include <stddef.h>
#include <stdio.h>
#include <time.h>
#include <math.h>
#include <stdlib.h>
#include <algorithm>
#include <string>
#include <deque>
#include "benchmark_results.h"
#include "benchmark_timer.h"
#include "benchmark_typenames.h"
#include "benchmark_algorithms.h"

/******************************************************************************/
/******************************************************************************/

// this constant may need to be adjusted to give reasonable minimum times
// For best results, times should be about 1.0 seconds for the minimum test run
int iterations = 6000000;


// 4000 items, or about 32k of data
// this is intended to remain within the L2 cache of most common CPUs
const int SIZE = 4000;


// initial value for filling our arrays, may be changed from the command line
int32_t init_value2 = 99;
int32_t init_value = 11;
int32_t init_value3 = 2;

/******************************************************************************/

#include "benchmark_shared_tests.h"

/******************************************************************************/
/******************************************************************************/

template <typename T>
struct max_std_functor {
    static inline T do_shift(T a, T b)
        {
        return std::max(a,b);
        }
};

/******************************************************************************/

// TODO - need reverse!
template <typename T>
struct max_functor1 {
    static inline T do_shift(T a, T b)
        {
        if (a > b)
            return a;
        else
            return b;
        }
};

/******************************************************************************/

// TODO - need reverse!
template <typename T>
struct max_functor2 {
    static inline T do_shift(T a, T b)
        {
        return ( a > b ) ? a : b;
        }
};

/******************************************************************************/

// this will only work for types where bitwise and, negation, and assignment of a bool work (integers)
template <typename T>
struct max_functor3 {
    static inline T do_shift(T a, T b)
        {
        return a - ((a - b) & -T(a < b));
        }
};

/******************************************************************************/

// this will only work for types where bitwise and, negation, and assignment of a bool work (integers)
template <typename T>
struct max_functor5 {
    static inline T do_shift(T a, T b)
        {
        return a ^ ((a ^ b) & -T(a < b));
        }
};

/******************************************************************************/

// reverse
template <typename T>
struct max_functor8 {
    static inline T do_shift(T a, T b)
        {
        if (a < b)
            return b;
        else
            return a;
        }
};

/******************************************************************************/

// reverse
template <typename T>
struct max_functor9 {
    static inline T do_shift(T a, T b)  {
        return ( a < b ) ? b : a;
    }
};

/******************************************************************************/
/******************************************************************************/

template <typename T>
struct min_std_functor {
    static inline T do_shift(T a, T b) {
        return std::min(a,b);
        }
};

/******************************************************************************/

// TODO - need reverse!
template <typename T>
struct min_functor1 {
    static inline T do_shift(T a, T b)  {
        if (a < b)
            return a;
        else
            return b;
    }
};

/******************************************************************************/

// TODO - need reverse!
template <typename T>
struct min_functor2 {
    static inline T do_shift(T a, T b) {
        return ( a < b ) ? a : b;
    }
};

/******************************************************************************/

// this will only work for types where bitwise and, negation, and assignment of a bool work (integers)
template <typename T>
struct min_functor3 {
    static inline T do_shift(T a, T b) {
        return b + ((a - b) & -T(a < b));
    }
};

/******************************************************************************/

// this will only work for types where bitwise and, negation, and assignment of a bool work (integers)
template <typename T>
struct min_functor5 {
    static inline T do_shift(T a, T b)
        {
        return b ^ ((a ^ b) & -T(a < b));
        }
};

/******************************************************************************/

// reverse
template <typename T>
struct min_functor8 {
    static inline T do_shift(T a, T b)
        {
        if (a > b)
            return b;
        else
            return a;
        }
};

/******************************************************************************/

// reverse
template <typename T>
struct min_functor9 {
    static inline T do_shift(T a, T b)
        {
        return ( a > b ) ? b : a;
        }
};

/******************************************************************************/
/******************************************************************************/

template <typename T>
struct pin_std_functor {
    static inline T do_shift(T low, T value, T high)
        {
        return std::max( low, std::min( value, high ) );
        }
};

/******************************************************************************/


template <typename T>
struct pin_functor1 {
    static inline T do_shift(T low, T value, T high)
        {
        return max_functor1<T>::do_shift( low, min_functor1<T>::do_shift( value, high ) );
        }
};

/******************************************************************************/

template <typename T>
struct pin_functor2 {
    static inline T do_shift(T low, T value, T high)
        {
        return max_functor2<T>::do_shift( low, min_functor2<T>::do_shift( value, high ) );
        }
};

/******************************************************************************/

template <typename T>
struct pin_functor3 {
    static inline T do_shift(T low, T value, T high)
        {
        return max_functor3<T>::do_shift( low, min_functor3<T>::do_shift( value, high ) );
        }
};

/******************************************************************************/

template <typename T>
struct pin_functor5 {
    static inline T do_shift(T low, T value, T high)
        {
        return max_functor5<T>::do_shift( low, min_functor5<T>::do_shift( value, high ) );
        }
};

/******************************************************************************/

template <typename T>
struct pin_functor8 {
    static inline T do_shift(T low, T value, T high)
        {
        return max_functor8<T>::do_shift( low, min_functor8<T>::do_shift( value, high ) );
        }
};

/******************************************************************************/

template <typename T>
struct pin_functor9 {
    static inline T do_shift(T low, T value, T high)
        {
        return max_functor9<T>::do_shift( low, min_functor9<T>::do_shift( value, high ) );
        }
};

/******************************************************************************/
/******************************************************************************/

template <typename T>
inline void check_sum(T result, const std::string &label) {
    T temp = T(SIZE * T(init_value)) ;
    if (!tolerance_equal<T>(result,temp))
        printf("test %s failed\n", label.c_str());
}

/******************************************************************************/

template <typename T>
inline void check_max_sum(T result, const std::string &label) {
    T temp = T(SIZE * T((init_value > init_value2) ? init_value : init_value2) ) ;
    if (!tolerance_equal<T>(result,temp))
        printf("test %s failed\n", label.c_str());
}

/******************************************************************************/

template <typename T>
inline void check_min_sum(T result, const std::string &label) {
    T temp = T(SIZE * T((init_value < init_value2) ? init_value : init_value2) ) ;
    if (!tolerance_equal<T>(result,temp))
        printf("test %s failed\n", label.c_str());
}

/******************************************************************************/

template <typename T, typename Shifter>
void validate_minmax(T a, T b, T expected, std::string &label) {
    if (expected != Shifter::do_shift( a, b ))
        printf("test %s failed validation\n", label.c_str());
    if (expected != Shifter::do_shift( b, a ))
        printf("test %s failed reverse validation\n", label.c_str());
}

/******************************************************************************/

template <typename T, typename Shifter>
void validate_max(std::string &label) {
    if (isSigned<T>())
        validate_minmax<T,Shifter>(80,-100,80,label);
    if (isSigned<T>())
        validate_minmax<T,Shifter>(127,-127,127,label);
    validate_minmax<T,Shifter>(1,2,2,label);
    validate_minmax<T,Shifter>(127,0,127,label);
    validate_minmax<T,Shifter>(0,1,1,label);
    
    validate_minmax<T,Shifter>(0,0,0,label);
    validate_minmax<T,Shifter>(1,1,1,label);
    validate_minmax<T,Shifter>(4,4,4,label);
    validate_minmax<T,Shifter>(64,64,64,label);
    validate_minmax<T,Shifter>(127,127,127,label);
    validate_minmax<T,Shifter>(T(-1),T(-1),T(-1),label);
}

/******************************************************************************/

template <typename T, typename Shifter>
void validate_min(std::string &label) {
    if (isSigned<T>())
        validate_minmax<T,Shifter>(80,-100,-100,label);
    if (isSigned<T>())
        validate_minmax<T,Shifter>(127,-127,-127,label);
    validate_minmax<T,Shifter>(1,2,1,label);
    validate_minmax<T,Shifter>(127,0,0,label);
    validate_minmax<T,Shifter>(0,1,0,label);
    
    validate_minmax<T,Shifter>(0,0,0,label);
    validate_minmax<T,Shifter>(1,1,1,label);
    validate_minmax<T,Shifter>(4,4,4,label);
    validate_minmax<T,Shifter>(64,64,64,label);
    validate_minmax<T,Shifter>(127,127,127,label);
    validate_minmax<T,Shifter>(T(-1),T(-1),T(-1),label);
}

/******************************************************************************/

template <typename T, typename Shifter>
void validate_pin(T lo, T val, T hi, T expected, std::string &label) {
    if (expected != Shifter::do_shift( lo, val, hi ))
        printf("test %s failed validation\n", label.c_str());
}

/******************************************************************************/

template <typename T, typename Shifter>
void validate_pin(std::string &label) {
    validate_pin<T,Shifter>(1,2,3,2,label);
    validate_pin<T,Shifter>(0,42,127,42,label);
    validate_pin<T,Shifter>(0,1,2,1,label);
    validate_pin<T,Shifter>(0,99,2,2,label);
    validate_pin<T,Shifter>(99,0,127,99,label);
    validate_pin<T,Shifter>(1,0,127,1,label);
    validate_pin<T,Shifter>(126,1,127,126,label);
    validate_pin<T,Shifter>(0,126,127,126,label);
    
    validate_pin<T,Shifter>(0,0,0,0,label);
    validate_pin<T,Shifter>(1,1,1,1,label);
    validate_pin<T,Shifter>(4,4,4,4,label);
    validate_pin<T,Shifter>(64,64,64,64,label);
    validate_pin<T,Shifter>(127,127,127,127,label);
    validate_pin<T,Shifter>(T(-1),T(-1),T(-1),T(-1),label);
}

/******************************************************************************/
/******************************************************************************/

static std::deque<std::string> gLabels;

template <typename T, typename Shifter>
void test_maximum(T* first, T* first2, int count, std::string label) {
    int i;
    
    validate_max<T,Shifter>(label);

    start_timer();

    for(i = 0; i < iterations; ++i) {
        T result = 0;
        for (int n = 0; n < count; ++n) {
            result += Shifter::do_shift( first[n], first2[n] );
        }
        check_max_sum(result, label);
    }

    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

template <typename T, typename Shifter>
void test_minimum(T* first, T* first2, int count, std::string label) {
    int i;
    
    validate_min<T,Shifter>(label);

    start_timer();

    for(i = 0; i < iterations; ++i) {
        T result = 0;
        for (int n = 0; n < count; ++n) {
            result += Shifter::do_shift( first[n], first2[n] );
        }
        check_min_sum(result, label);
    }

    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

template <typename T, typename Shifter>
void test_pin(T* first, T* first2, T* first3, int count, std::string label) {
    int i;
    
    validate_pin<T,Shifter>(label);

    start_timer();

    for(i = 0; i < iterations; ++i) {
        T result = 0;
        for (int n = 0; n < count; ++n) {
            result += Shifter::do_shift( first3[n], first[n], first2[n] );
        }
        check_sum(result, label);
    }

    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/
/******************************************************************************/

template<typename T>
void TestInts()
{
    std::string myTypeName( getTypeName<T>() );
    
    gLabels.clear();
    
    T data [ SIZE ];
    T data_larger [ SIZE ];
    T data_smaller [ SIZE ];
    
    ::fill( data, data+SIZE, T(init_value) );
    ::fill( data_larger, data_larger+SIZE, T(init_value2) );
    ::fill( data_smaller, data_smaller+SIZE, T(init_value3) );
    
    test_maximum<T, max_std_functor<T> >( data, data_larger, SIZE, myTypeName + " std::max");
    test_maximum<T, max_functor1<T> >( data, data_larger, SIZE, myTypeName + " maximum1");
    test_maximum<T, max_functor2<T> >( data, data_larger, SIZE, myTypeName + " maximum2");
    test_maximum<T, max_functor3<T> >( data, data_larger, SIZE, myTypeName + " maximum3");    // int only
    test_maximum<T, max_functor5<T> >( data, data_larger, SIZE, myTypeName + " maximum5");    // int only
    test_maximum<T, max_functor8<T> >( data, data_larger, SIZE, myTypeName + " maximum8");
    test_maximum<T, max_functor9<T> >( data, data_larger, SIZE, myTypeName + " maximum9");
    
    
    test_minimum<T, min_std_functor<T> >( data, data_larger, SIZE, myTypeName + " std::min");
    test_minimum<T, min_functor1<T> >( data, data_larger, SIZE, myTypeName + " minimum1");
    test_minimum<T, min_functor2<T> >( data, data_larger, SIZE, myTypeName + " minimum2");
    test_minimum<T, min_functor3<T> >( data, data_larger, SIZE, myTypeName + " minimum3");    // int only
    test_minimum<T, min_functor5<T> >( data, data_larger, SIZE, myTypeName + " minimum5");    // int only
    test_minimum<T, min_functor8<T> >( data, data_larger, SIZE, myTypeName + " minimum8");
    test_minimum<T, min_functor9<T> >( data, data_larger, SIZE, myTypeName + " minimum9");
    
    
    test_pin<T, pin_std_functor<T> >( data, data_larger, data_smaller, SIZE, myTypeName + " std::min,max");
    test_pin<T, pin_functor1<T> >( data, data_larger, data_smaller, SIZE, myTypeName + " pin1");
    test_pin<T, pin_functor2<T> >( data, data_larger, data_smaller, SIZE, myTypeName + " pin2");
    test_pin<T, pin_functor3<T> >( data, data_larger, data_smaller, SIZE, myTypeName + " pin3");    // int only
    test_pin<T, pin_functor5<T> >( data, data_larger, data_smaller, SIZE, myTypeName + " pin5");    // int only
    test_pin<T, pin_functor8<T> >( data, data_larger, data_smaller, SIZE, myTypeName + " pin8");
    test_pin<T, pin_functor9<T> >( data, data_larger, data_smaller, SIZE, myTypeName + " pin9");


    std::string temp1( myTypeName + " minmax" );
    summarize(temp1.c_str(), SIZE, iterations, kDontShowGMeans, kDontShowPenalty );
}

/******************************************************************************/

// value2 > value > value3
template<typename T>
void TestFloats()
{
    std::string myTypeName( getTypeName<T>() );
    
    gLabels.clear();
    
    T data [ SIZE ];
    T data_larger [ SIZE ];
    T data_smaller [ SIZE ];
    
    ::fill( data, data+SIZE, T(init_value) );
    ::fill( data_larger, data_larger+SIZE, T(init_value2) );
    ::fill( data_smaller, data_smaller+SIZE, T(init_value3) );
    
    test_maximum<T, max_std_functor<T> >( data, data_larger, SIZE, myTypeName + " std::max");
    test_maximum<T, max_functor1<T> >( data, data_larger, SIZE, myTypeName + " maximum1");
    test_maximum<T, max_functor2<T> >( data, data_larger, SIZE, myTypeName + " maximum2");
    test_maximum<T, max_functor8<T> >( data, data_larger, SIZE, myTypeName + " maximum8");
    test_maximum<T, max_functor9<T> >( data, data_larger, SIZE, myTypeName + " maximum9");
    
    
    test_minimum<T, min_std_functor<T> >( data, data_larger, SIZE, myTypeName + " std::min");
    test_minimum<T, min_functor1<T> >( data, data_larger, SIZE, myTypeName + " minimum1");
    test_minimum<T, min_functor2<T> >( data, data_larger, SIZE, myTypeName + " minimum2");
    test_minimum<T, min_functor8<T> >( data, data_larger, SIZE, myTypeName + " minimum8");
    test_minimum<T, min_functor9<T> >( data, data_larger, SIZE, myTypeName + " minimum9");
    
    
    test_pin<T, pin_std_functor<T> >( data, data_larger, data_smaller, SIZE, myTypeName + " std::min,max");
    test_pin<T, pin_functor1<T> >( data, data_larger, data_smaller, SIZE, myTypeName + " pin1");
    test_pin<T, pin_functor2<T> >( data, data_larger, data_smaller, SIZE, myTypeName + " pin2");
    test_pin<T, pin_functor8<T> >( data, data_larger, data_smaller, SIZE, myTypeName + " pin8");
    test_pin<T, pin_functor9<T> >( data, data_larger, data_smaller, SIZE, myTypeName + " pin9");


    std::string temp1( myTypeName + " minmax" );
    summarize(temp1.c_str(), SIZE, iterations, kDontShowGMeans, kDontShowPenalty );
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
    if (argc > 2) init_value = (int32_t) atoi(argv[2]);
    if (argc > 3) init_value2 = (int32_t) atoi(argv[3]);
    if (argc > 4) init_value3 = (int32_t) atoi(argv[4]);
    
    
    // ensure that value2 is larger than value3
    if (init_value3 >= init_value2) {
        printf("bad init_value3, adjusting\n");
        init_value3 = init_value2 - 1;
    }
    
    // ensure that value is larger than value3
    if (init_value <= init_value3) {
        printf("bad init_value, adjusting\n");
        init_value = init_value3 + 1;
    }
    
    // ensure that value2 is larger than value
    if (init_value2 <= init_value) {
        printf("bad init_value2, adjusting\n");
        init_value2 = init_value + 1;
    }

    // now we have   value2 > value > value3


    TestInts<uint8_t>();
    TestInts<int8_t>();
    TestInts<uint16_t>();
    TestInts<int16_t>();
    
    iterations /= 2;
    TestInts<uint32_t>();
    TestInts<int32_t>();
    
    iterations /= 2;
    TestInts<uint64_t>();
    TestInts<int64_t>();

    TestFloats<float>();
    TestFloats<double>();
    TestFloats<long double>();

    return 0;
}

// the end
/******************************************************************************/
/******************************************************************************/
