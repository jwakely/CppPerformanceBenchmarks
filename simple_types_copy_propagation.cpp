/*
    Copyright 2008 Adobe Systems Incorporated
    Copyright 2018 Chris Cox
    Distributed under the MIT License (see accompanying file LICENSE_1_0_0.txt
    or a copy at http://stlab.adobe.com/licenses.html )


Goal:  Test compiler optimizations related to propagation of copies of simple language defined types.

Assumptions:

    1) The compiler will propagate copies of values through expressions to simplify them.
        aka: copy propagation
        also related to value numbering


TODO - need more complex examples.

*/

/******************************************************************************/

#include "benchmark_stdint.hpp"
#include <cstddef>
#include <cstdio>
#include <ctime>
#include <cstdlib>
#include <string>
#include <deque>
#include "benchmark_results.h"
#include "benchmark_timer.h"
#include "benchmark_typenames.h"

/******************************************************************************/

// this copy may need to be adjusted to give reasonable minimum times
// For best results, times should be about 1.0 seconds for the minimum test run
int iterations = 4000000;


// 8000 items, or between 8k and 64k of data
// this is intended to remain within the L2 cache of most common CPUs
const int SIZE = 8000;


// initial value for filling our arrays, may be changed from the command line
double init_value = 3.0;

/******************************************************************************/

#include "benchmark_shared_tests.h"

/******************************************************************************/

template <typename T>
    struct custom_copy_chain {
      static T do_shift(T input) {
        T a = input;
        T b = a;
        T c = b;
        T d = c;
        T e = d;
        T f = e;
        T g = f;
        T h = g;
        T i = h;
        T j = i;
        T k = j;
        T l = k;
        T m = l;
        return m;
        }
    };

/******************************************************************************/

template <typename T>
    struct custom_copy_chain2 {
      static T do_shift(T input) {
        T a = custom_copy_chain<T>::do_shift(input);
        T b = custom_copy_chain<T>::do_shift(a);
        T c = custom_copy_chain<T>::do_shift(b);
        T d = custom_copy_chain<T>::do_shift(c);
        T e = custom_copy_chain<T>::do_shift(d);
        T f = custom_copy_chain<T>::do_shift(e);
        T g = custom_copy_chain<T>::do_shift(f);
        T h = custom_copy_chain<T>::do_shift(g);
        return h;
        }
    };

/******************************************************************************/

template <typename T>
    struct custom_copy_chain3 {
      static T do_shift(T input) {
        T a = custom_copy_chain2<T>::do_shift(input);
        T b = custom_copy_chain2<T>::do_shift(a);
        T c = custom_copy_chain2<T>::do_shift(b);
        T d = custom_copy_chain2<T>::do_shift(c);
        T e = custom_copy_chain2<T>::do_shift(d);
        T f = custom_copy_chain2<T>::do_shift(e);
        T g = custom_copy_chain2<T>::do_shift(f);
        T h = custom_copy_chain2<T>::do_shift(g);
        return h;
        }
    };

/******************************************************************************/

template <typename T>
    struct custom_copy_chain4 {
      static T do_shift(T input) {
        T a = custom_copy_chain3<T>::do_shift(input);
        T b = custom_copy_chain3<T>::do_shift(a);
        T c = custom_copy_chain3<T>::do_shift(b);
        T d = custom_copy_chain3<T>::do_shift(c);
        T e = custom_copy_chain3<T>::do_shift(d);
        T f = custom_copy_chain3<T>::do_shift(e);
        T g = custom_copy_chain3<T>::do_shift(f);
        T h = custom_copy_chain3<T>::do_shift(g);
        return h;
        }
    };

/******************************************************************************/

template <typename T>
    struct custom_copy_chain_dead {
      static T do_shift(T input) {
        T a = input;
        input += 2;
        T b = a;
        a -= 2;
        T c = b;
        b *= 7;
        T d = c;
        c /= 11;
        T e = d;
        d += 33;
        T f = e;
        e -= 7;
        input += 77;
        T g = f;
        f *= 8;
        T h = g;
        g /= 13;
        T i = h;
        h += 3;
        T j = i;
        i -= 7;
        T k = j;
        j *= 17;
        T l = k;
        k /= 23;
        T m = l;
        l += (a/7 + b*13 + c/11 + d*8) + input;
        return m;
        }
    };

/******************************************************************************/

template <typename T>
    struct custom_copy_chain_dead2 {
      static T do_shift(T input) {
        T a = custom_copy_chain_dead<T>::do_shift(input);
        T b = custom_copy_chain_dead<T>::do_shift(a);
        T c = custom_copy_chain_dead<T>::do_shift(b);
        T d = custom_copy_chain_dead<T>::do_shift(c);
        T e = custom_copy_chain_dead<T>::do_shift(d);
        T f = custom_copy_chain_dead<T>::do_shift(e);
        T g = custom_copy_chain_dead<T>::do_shift(f);
        T h = custom_copy_chain_dead<T>::do_shift(g);
        return h;
        }
    };

/******************************************************************************/

template <typename T>
    struct custom_copy_chain_dead3 {
      static T do_shift(T input) {
        T a = custom_copy_chain_dead2<T>::do_shift(input);
        T b = custom_copy_chain_dead2<T>::do_shift(a);
        T c = custom_copy_chain_dead2<T>::do_shift(b);
        T d = custom_copy_chain_dead2<T>::do_shift(c);
        T e = custom_copy_chain_dead2<T>::do_shift(d);
        T f = custom_copy_chain_dead2<T>::do_shift(e);
        T g = custom_copy_chain_dead2<T>::do_shift(f);
        T h = custom_copy_chain_dead2<T>::do_shift(g);
        return h;
        }
    };

/******************************************************************************/

template <typename T>
    struct custom_copy_chain_dead4 {
      static T do_shift(T input) {
        T a = custom_copy_chain_dead3<T>::do_shift(input);
        T b = custom_copy_chain_dead3<T>::do_shift(a);
        T c = custom_copy_chain_dead3<T>::do_shift(b);
        T d = custom_copy_chain_dead3<T>::do_shift(c);
        T e = custom_copy_chain_dead3<T>::do_shift(d);
        T f = custom_copy_chain_dead3<T>::do_shift(e);
        T g = custom_copy_chain_dead3<T>::do_shift(f);
        T h = custom_copy_chain_dead3<T>::do_shift(g);
        return h;
        }
    };

/******************************************************************************/

// all branching and tests should be removed, because both directions assign the same value
template <typename T>
    struct custom_copy_branched {
      static T do_shift(T input) {
        T a = input;
        T b = 42;
        T c = 99;
        T d = 11;
        T e = 22;
        T f = 33;
        
        if (input == 0)
            a = input;
        else
            a = input;
        
        if (a != 0)
            b = a;
        else
            b = input;
        
        if (b > 99)
            c = a;
        else
            c = b;
        
        if (c < 0)
            d = b;
        else
            d = c;
        
        if (d > 867)
            e = d;
        else
            e = a;
        
        if (e == 5309)
            f = c;
        else
            f = e;
        
        return f;
        }
    };

/******************************************************************************/

template <typename T>
    struct custom_copy_branched2 {
      static T do_shift(T input) {
        T a = custom_copy_branched<T>::do_shift(input);
        T b = custom_copy_branched<T>::do_shift(a);
        T c = custom_copy_branched<T>::do_shift(b);
        T d = custom_copy_branched<T>::do_shift(c);
        T e = custom_copy_branched<T>::do_shift(d);
        T f = custom_copy_branched<T>::do_shift(e);
        T g = custom_copy_branched<T>::do_shift(f);
        T h = custom_copy_branched<T>::do_shift(g);
        return h;
        }
    };

/******************************************************************************/

template <typename T>
    struct custom_copy_branched3 {
      static T do_shift(T input) {
        T a = custom_copy_branched2<T>::do_shift(input);
        T b = custom_copy_branched2<T>::do_shift(a);
        T c = custom_copy_branched2<T>::do_shift(b);
        T d = custom_copy_branched2<T>::do_shift(c);
        T e = custom_copy_branched2<T>::do_shift(d);
        T f = custom_copy_branched2<T>::do_shift(e);
        T g = custom_copy_branched2<T>::do_shift(f);
        T h = custom_copy_branched2<T>::do_shift(g);
        return h;
        }
    };

/******************************************************************************/

template <typename T>
    struct custom_copy_branched4 {
      static T do_shift(T input) {
        T a = custom_copy_branched3<T>::do_shift(input);
        T b = custom_copy_branched3<T>::do_shift(a);
        T c = custom_copy_branched3<T>::do_shift(b);
        T d = custom_copy_branched3<T>::do_shift(c);
        T e = custom_copy_branched3<T>::do_shift(d);
        T f = custom_copy_branched3<T>::do_shift(e);
        T g = custom_copy_branched3<T>::do_shift(f);
        T h = custom_copy_branched3<T>::do_shift(g);
        return h;
        }
    };

/******************************************************************************/

// all branching and tests should be removed, because both directions assign the same value
template <typename T>
    struct custom_copy_branched_dead {
      static T do_shift(T input) {
        T a = input;
        T b = 42;
        T c = 99;
        T d = 11;
        T e = 22;
        T f = 33;
        
        if ((input / 3) == 0)
            a = input;
        else
            a = input;
        
        if ((a/5) != 0)
            b = a;
        else
            b = input;
        
        input /= 11;
        
        if (((b+7)/2) > (99*a))
            c = a;
        else
            c = b;
        
        a *= 13;
        
        if (c < input)
            d = b;
        else
            d = c;
        
        b += 77;
        
        if ((b*11) > 867)
            e = d;
        else
            e = c;
        
        if ((e*e*e*e+c) == 5309)
            f = d;
        else
            f = e;
        
        return f;
        }
    };

/******************************************************************************/

template <typename T>
    struct custom_copy_branched_dead2 {
      static T do_shift(T input) {
        T a = custom_copy_branched_dead<T>::do_shift(input);
        T b = custom_copy_branched_dead<T>::do_shift(a);
        T c = custom_copy_branched_dead<T>::do_shift(b);
        T d = custom_copy_branched_dead<T>::do_shift(c);
        T e = custom_copy_branched_dead<T>::do_shift(d);
        T f = custom_copy_branched_dead<T>::do_shift(e);
        T g = custom_copy_branched_dead<T>::do_shift(f);
        T h = custom_copy_branched_dead<T>::do_shift(g);
        return h;
        }
    };

/******************************************************************************/

template <typename T>
    struct custom_copy_branched_dead3 {
      static T do_shift(T input) {
        T a = custom_copy_branched_dead2<T>::do_shift(input);
        T b = custom_copy_branched_dead2<T>::do_shift(a);
        T c = custom_copy_branched_dead2<T>::do_shift(b);
        T d = custom_copy_branched_dead2<T>::do_shift(c);
        T e = custom_copy_branched_dead2<T>::do_shift(d);
        T f = custom_copy_branched_dead2<T>::do_shift(e);
        T g = custom_copy_branched_dead2<T>::do_shift(f);
        T h = custom_copy_branched_dead2<T>::do_shift(g);
        return h;
        }
    };

/******************************************************************************/

template <typename T>
    struct custom_copy_branched_dead4 {
      static T do_shift(T input) {
        T a = custom_copy_branched_dead3<T>::do_shift(input);
        T b = custom_copy_branched_dead3<T>::do_shift(a);
        T c = custom_copy_branched_dead3<T>::do_shift(b);
        T d = custom_copy_branched_dead3<T>::do_shift(c);
        T e = custom_copy_branched_dead3<T>::do_shift(d);
        T f = custom_copy_branched_dead3<T>::do_shift(e);
        T g = custom_copy_branched_dead3<T>::do_shift(f);
        T h = custom_copy_branched_dead3<T>::do_shift(g);
        return h;
        }
    };

/******************************************************************************/
/******************************************************************************/

static std::deque<std::string> gLabels;

template <typename T, typename Shifter>
void test_constant(T* first, int count, const std::string label) {
    int i;

    start_timer();

    for(i = 0; i < iterations; ++i) {
        T result = 0;
        for (int n = 0; n < count; ++n) {
            result += Shifter::do_shift( first[n] );
        }
        check_shifted_sum<T, Shifter>(result);
    }

    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/
/******************************************************************************/

template< typename T >
void TestOneType()
{
    gLabels.clear();
    
    std::string myTypeName( getTypeName<T>() );

    T data[SIZE];
    ::fill(data, data+SIZE, T(init_value));
    
    test_constant<T, custom_identity<T> >(data,SIZE,myTypeName + " identity");
    test_constant<T, custom_copy_chain<T> >(data,SIZE,myTypeName + " copy chain");
    test_constant<T, custom_copy_chain2<T> >(data,SIZE,myTypeName + " copy chain2");
    test_constant<T, custom_copy_chain2<T> >(data,SIZE,myTypeName + " copy chain3");
    test_constant<T, custom_copy_chain4<T> >(data,SIZE,myTypeName + " copy chain4");
    test_constant<T, custom_copy_chain_dead<T> >(data,SIZE,myTypeName + " copy chain dead");
    test_constant<T, custom_copy_chain_dead2<T> >(data,SIZE,myTypeName + " copy chain dead2");
    test_constant<T, custom_copy_chain_dead3<T> >(data,SIZE,myTypeName + " copy chain dead3");
    test_constant<T, custom_copy_chain_dead4<T> >(data,SIZE,myTypeName + " copy chain dead4");
    test_constant<T, custom_copy_branched<T> >(data,SIZE,myTypeName + " copy branched");
    test_constant<T, custom_copy_branched2<T> >(data,SIZE,myTypeName + " copy branched2");
    test_constant<T, custom_copy_branched3<T> >(data,SIZE,myTypeName + " copy branched3");
    test_constant<T, custom_copy_branched4<T> >(data,SIZE,myTypeName + " copy branched4");
    test_constant<T, custom_copy_branched_dead<T> >(data,SIZE,myTypeName + " copy branched dead");
    test_constant<T, custom_copy_branched_dead2<T> >(data,SIZE,myTypeName + " copy branched dead2");
    test_constant<T, custom_copy_branched_dead3<T> >(data,SIZE,myTypeName + " copy branched dead3");
    test_constant<T, custom_copy_branched_dead4<T> >(data,SIZE,myTypeName + " copy branched dead4");
    
    std::string temp1( myTypeName + " simple copy propagation" );
    summarize(temp1.c_str(), SIZE, iterations, kDontShowGMeans, kDontShowPenalty );

}

/******************************************************************************/

int main(int argc, char** argv) {
    
    // output command for documentation:
    int i;
    for (i = 0; i < argc; ++i)
        printf("%s ", argv[i] );
    printf("\n");

    if (argc > 1) iterations = atoi(argv[1]);
    if (argc > 2) init_value = fabs( (double) atof(argv[2]) );


    TestOneType<int8_t>();
    TestOneType<uint8_t>();
    TestOneType<int16_t>();
    TestOneType<uint16_t>();
    TestOneType<int32_t>();
    TestOneType<uint32_t>();
    
    iterations /= 10;
    
    TestOneType<int64_t>();
    TestOneType<uint64_t>();
    TestOneType<float>();
    TestOneType<double>();
    TestOneType<long double>();
    
    return 0;
}

// the end
/******************************************************************************/
/******************************************************************************/
