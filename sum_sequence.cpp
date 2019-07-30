/*
    Copyright 2008 Adobe Systems Incorporated
    Copyright 2019 Chris Cox
    Distributed under the MIT License (see accompanying file LICENSE_1_0_0.txt
    or a copy at http://stlab.adobe.com/licenses.html )


Goal: test performance of various idioms for summing a sequence.


Assumptions:
    1) The compiler will optimize summation operations.
 
    2) std::accumulate will be well optimized for all types and containers.
    
    3) The compiler may recognize ineffecient summation idioms and substitute efficient methods.



NOTE - MSVC generates dozens of bogus precision loss error messages about std::accumulate
    LLVM and gcc are fine.
    Looks like a problem in header source <numeric>, or possibly with compiler handling if statements in template




TODO - test other containers
    vector
    deque
    list
    forward_list

TODO - create forward iterator friendly versions of each accumulator

NOTE - Simple cache hinting did not help on Intel i7
        The built-in linear predictor is good enough.

NOTE - std::reduce not available until C++17 (which is not yet stable on all available compilers)

*/

/******************************************************************************/

#include "benchmark_stdint.hpp"
#include <cstddef>
#include <cstdio>
#include <ctime>
#include <cmath>
#include <cstdlib>
#include <numeric>
#include <deque>
#include <string>
#include <iostream>
#include "benchmark_results.h"
#include "benchmark_timer.h"
#include "benchmark_algorithms.h"
#include "benchmark_typenames.h"

/******************************************************************************/
/******************************************************************************/

// this constant may need to be adjusted to give reasonable minimum times
// For best results, times should be about 1.0 seconds for the minimum test run
int iterations = 6000000;


// 8000 items, or between 8 and 64k of data
// this is intended to remain within the L2 cache of most common CPUs
const int SIZE = 8000;


// initial value for filling our arrays, may be changed from the command line
int32_t init_value = 3;

/******************************************************************************/
/******************************************************************************/

template<typename T>
inline void check_sum(T result, const std::string &label) {
    const T expected = T(SIZE) * T(init_value);
    if ( result != expected )
        std::cout << "test " << label << " failed, got " << result << " instead of " << expected << "\n";
}

/******************************************************************************/
/******************************************************************************/

template <typename Iter, typename T>
struct accumulate_std {
    T operator()( Iter first, const size_t count )
    {
        auto end = first + count;
        return std::accumulate( first, end, T(0) );
    }
};

/******************************************************************************/
/******************************************************************************/

#if _LIBCPP_STD_VER > 14
template <typename Iter, typename T>
struct reduce_add_std {
    T operator()( Iter first, const size_t count )
    {
        auto end = first + count;
        return std::reduce( first, end, T(0) );
    }
};
#endif

/******************************************************************************/

template <typename Iter, typename T>
struct accumulate1 {
    T operator()( Iter first, const size_t count )
    {
        T sum(0);
        for (size_t j = 0; j < count; ++j) {
            sum = sum + first[j];
        }
        return sum;
    }
};

/******************************************************************************/

template <typename Iter, typename T>
struct accumulate2 {
    T operator()( Iter first, const size_t count )
    {
        T sum(0);
        auto end = first+count;
        for ( ;first != end; ++first) {
            sum += *first;
        }
        return sum;
    }
};

/******************************************************************************/

// unroll 2X
template <typename Iter, typename T>
struct accumulate3 {
    T operator()( Iter first, const size_t count )
    {
        T sum(0);
        size_t j;
        
        for (j = 0; j < (count - 1); j += 2) {
            sum += first[j+0];
            sum += first[j+1];
        }

        for (; j < count; ++j) {
            sum += first[j];
        }
        return sum;
    }
};

/******************************************************************************/

// unroll 2X and use multiple accumulation variables
template <typename Iter, typename T>
struct accumulate4 {
    T operator()( Iter first, const size_t count )
    {
        T sum(0);
        T sum1(0);
        size_t j;
        
        for (j = 0; j < (count - 1); j += 2) {
            sum += first[j+0];
            sum1 += first[j+1];
        }

        for (; j < count; ++j) {
            sum += first[j];
        }

        sum += sum1;
        
        return sum;
    }
};

/******************************************************************************/

// unroll 4X
template <typename Iter, typename T>
struct accumulate5 {
    T operator()( Iter first, const size_t count )
    {
        T sum(0);
        size_t j;
        
        for (j = 0; j < (count - 3); j += 4) {
            sum += first[j+0];
            sum += first[j+1];
            sum += first[j+2];
            sum += first[j+3];
        }

        for (; j < count; ++j) {
            sum += first[j];
        }
        return sum;
    }
};

/******************************************************************************/

// unroll 4X and use multiple accumulation variables
template <typename Iter, typename T>
struct accumulate6 {
    T operator()( Iter first, const size_t count )
    {
        T sum(0);
        T sum1(0);
        T sum2(0);
        T sum3(0);
        size_t j;
        
        for (j = 0; j < (count - 3); j += 4) {
            sum  += first[j+0];
            sum1 += first[j+1];
            sum2 += first[j+2];
            sum3 += first[j+3];
        }

        for (; j < count; ++j) {
            sum += first[j];
        }

        sum += sum1 + sum2 + sum3;
        
        return sum;
    }
};

/******************************************************************************/

// unroll 8X
template <typename Iter, typename T>
struct accumulate7 {
    T operator()( Iter first, const size_t count )
    {
        T sum(0);
        size_t j;
        
        for (j = 0; j < (count - 7); j += 8) {
            sum += first[j+0];
            sum += first[j+1];
            sum += first[j+2];
            sum += first[j+3];
            sum += first[j+4];
            sum += first[j+5];
            sum += first[j+6];
            sum += first[j+7];
        }

        for (; j < count; ++j) {
            sum += first[j];
        }
        
        return sum;
    }
};

/******************************************************************************/

// unroll 8X and use multiple accumulation variables
template <typename Iter, typename T>
struct accumulate8 {
    T operator()( Iter first, const size_t count )
    {
        T sum(0);
        T sum1(0);
        T sum2(0);
        T sum3(0);
        size_t j;
        
        for (j = 0; j < (count - 7); j += 8) {
            sum  += first[j+0];
            sum1 += first[j+1];
            sum2 += first[j+2];
            sum3 += first[j+3];
            sum  += first[j+4];
            sum1 += first[j+5];
            sum2 += first[j+6];
            sum3 += first[j+7];
        }

        for (; j < count; ++j) {
            sum += first[j];
        }

        sum += sum1 + sum2 + sum3;
        
        return sum;
    }
};

/******************************************************************************/

// unroll 16X
template <typename Iter, typename T>
struct accumulate9 {
    T operator()( Iter first, const size_t count )
    {
        T sum(0);
        size_t j;
        
        for (j = 0; j < (count - 15); j += 16) {
            sum += first[j+0];
            sum += first[j+1];
            sum += first[j+2];
            sum += first[j+3];
            sum += first[j+4];
            sum += first[j+5];
            sum += first[j+6];
            sum += first[j+7];
            sum += first[j+8];
            sum += first[j+9];
            sum += first[j+10];
            sum += first[j+11];
            sum += first[j+12];
            sum += first[j+13];
            sum += first[j+14];
            sum += first[j+15];
        }
        
        for (; j < (count - 3); j += 4) {
            sum += first[j+0];
            sum += first[j+1];
            sum += first[j+2];
            sum += first[j+3];
        }

        for (; j < count; ++j) {
            sum += first[j];
        }
        
        return sum;
    }
};

/******************************************************************************/

// unroll 16X and use multiple accumulation variables
template <typename Iter, typename T>
struct accumulate10 {
    T operator()( Iter first, const size_t count )
    {
        T sum(0);
        T sum1(0);
        T sum2(0);
        T sum3(0);
        size_t j;
        
        for (j = 0; j < (count - 15); j += 16) {
            sum  += first[j+0];
            sum1 += first[j+1];
            sum2 += first[j+2];
            sum3 += first[j+3];
            sum  += first[j+4];
            sum1 += first[j+5];
            sum2 += first[j+6];
            sum3 += first[j+7];
            sum  += first[j+8];
            sum1 += first[j+9];
            sum2 += first[j+10];
            sum3 += first[j+11];
            sum  += first[j+12];
            sum1 += first[j+13];
            sum2 += first[j+14];
            sum3 += first[j+15];
        }
        
        for (; j < (count - 3); j += 4) {
            sum  += first[j+0];
            sum1 += first[j+1];
            sum2 += first[j+2];
            sum3 += first[j+3];
        }

        for (; j < count; ++j) {
            sum += first[j];
        }

        sum += sum1 + sum2 + sum3;
        
        return sum;
    }
};

/******************************************************************************/

// unroll 32X
template <typename Iter, typename T>
struct accumulate11 {
    T operator()( Iter first, const size_t count )
    {
        T sum(0);
        size_t j;
        
        for (j = 0; j < (count - 31); j += 32) {
            sum += first[j+0];
            sum += first[j+1];
            sum += first[j+2];
            sum += first[j+3];
            sum += first[j+4];
            sum += first[j+5];
            sum += first[j+6];
            sum += first[j+7];
            sum += first[j+8];
            sum += first[j+9];
            sum += first[j+10];
            sum += first[j+11];
            sum += first[j+12];
            sum += first[j+13];
            sum += first[j+14];
            sum += first[j+15];
            sum += first[j+16];
            sum += first[j+17];
            sum += first[j+18];
            sum += first[j+19];
            sum += first[j+20];
            sum += first[j+21];
            sum += first[j+22];
            sum += first[j+23];
            sum += first[j+24];
            sum += first[j+25];
            sum += first[j+26];
            sum += first[j+27];
            sum += first[j+28];
            sum += first[j+29];
            sum += first[j+30];
            sum += first[j+31];
        }
        
        for (; j < (count - 3); j += 4) {
            sum += first[j+0];
            sum += first[j+1];
            sum += first[j+2];
            sum += first[j+3];
        }

        for (; j < count; ++j) {
            sum += first[j];
        }
        
        return sum;
    }
};

/******************************************************************************/

// unroll 32X and use multiple accumulation variables
template <typename Iter, typename T>
struct accumulate12 {
    T operator()( Iter first, const size_t count )
    {
        T sum(0);
        T sum1(0);
        T sum2(0);
        T sum3(0);
        size_t j;
        
        for (j = 0; j < (count - 31); j += 32) {
            sum  += first[j+0];
            sum1 += first[j+1];
            sum2 += first[j+2];
            sum3 += first[j+3];
            sum  += first[j+4];
            sum1 += first[j+5];
            sum2 += first[j+6];
            sum3 += first[j+7];
            sum  += first[j+8];
            sum1 += first[j+9];
            sum2 += first[j+10];
            sum3 += first[j+11];
            sum  += first[j+12];
            sum1 += first[j+13];
            sum2 += first[j+14];
            sum3 += first[j+15];
            sum  += first[j+16];
            sum1 += first[j+17];
            sum2 += first[j+18];
            sum3 += first[j+19];
            sum  += first[j+20];
            sum1 += first[j+21];
            sum2 += first[j+22];
            sum3 += first[j+23];
            sum  += first[j+24];
            sum1 += first[j+25];
            sum2 += first[j+26];
            sum3 += first[j+27];
            sum  += first[j+28];
            sum1 += first[j+29];
            sum2 += first[j+30];
            sum3 += first[j+31];
        }
        
        for (; j < (count - 3); j += 4) {
            sum  += first[j+0];
            sum1 += first[j+1];
            sum2 += first[j+2];
            sum3 += first[j+3];
        }

        for (; j < count; ++j) {
            sum += first[j];
        }

        sum += sum1 + sum2 + sum3;
        
        return sum;
    }
};

/******************************************************************************/

// unroll 4X and use multiple accumulation variables made to look like vectors
template <typename Iter, typename T>
struct accumulate13 {
    T operator()( Iter first, const size_t count )
    {
        T sum[4] = { 0,0,0,0 };
        size_t j;
        
        for ( j = 0; j < (count - 3); j += 4) {
            sum[0] += first[j+0];
            sum[1] += first[j+1];
            sum[2] += first[j+2];
            sum[3] += first[j+3];
        }

        for (; j < count; ++j) {
            sum[0] += first[j];
        }

        sum[0] += sum[1] + sum[2] + sum[3];
        
        return sum[0];
    }
};

/******************************************************************************/

// unroll 8X and use multiple accumulation variables made to look like vectors
template <typename Iter, typename T>
struct accumulate14 {
    T operator()( Iter first, const size_t count )
    {
        T sum[8] = { 0,0,0,0, 0,0,0,0 };
        size_t j;
        
        for ( j = 0; j < (count - 7); j += 8) {
            sum[0] += first[j+0];
            sum[1] += first[j+1];
            sum[2] += first[j+2];
            sum[3] += first[j+3];
            sum[4] += first[j+4];
            sum[5] += first[j+5];
            sum[6] += first[j+6];
            sum[7] += first[j+7];
        }

        for (; j < count; ++j) {
            sum[0] += first[j];
        }

        sum[0] += sum[1] + sum[2] + sum[3];
        sum[4] += sum[5] + sum[6] + sum[7];
        sum[0] += sum[4];
        
        return sum[0];
    }
};

/******************************************************************************/

// unroll 16X and use multiple accumulation variables made to look like vectors
template <typename Iter, typename T>
struct accumulate15 {
    T operator()( Iter first, const size_t count )
    {
        T sum[8] = { 0,0,0,0, 0,0,0,0 };
        size_t j;
        
        for ( j = 0; j < (count - 15); j += 16) {
            sum[0] += first[j+0];
            sum[1] += first[j+1];
            sum[2] += first[j+2];
            sum[3] += first[j+3];
            sum[4] += first[j+4];
            sum[5] += first[j+5];
            sum[6] += first[j+6];
            sum[7] += first[j+7];
            sum[0] += first[j+8];
            sum[1] += first[j+9];
            sum[2] += first[j+10];
            sum[3] += first[j+11];
            sum[4] += first[j+12];
            sum[5] += first[j+13];
            sum[6] += first[j+14];
            sum[7] += first[j+15];
        }
        
        for ( ; j < (count - 3); j += 4) {
            sum[0] += first[j+0];
            sum[1] += first[j+1];
            sum[2] += first[j+2];
            sum[3] += first[j+3];
        }

        for (; j < count; ++j) {
            sum[0] += first[j];
        }

        sum[0] += sum[1] + sum[2] + sum[3];
        sum[4] += sum[5] + sum[6] + sum[7];
        sum[0] += sum[4];
        
        return sum[0];
    }
};

/******************************************************************************/

// unroll 32X and use multiple accumulation variables made to look like vectors
template <typename Iter, typename T>
struct accumulate16 {
    T operator()( Iter first, const size_t count )
    {
        T sum[8] = { 0,0,0,0, 0,0,0,0 };
        size_t j;
        
        for ( j = 0; j < (count - 31); j += 32) {
            sum[0] += first[j+0];
            sum[1] += first[j+1];
            sum[2] += first[j+2];
            sum[3] += first[j+3];
            sum[4] += first[j+4];
            sum[5] += first[j+5];
            sum[6] += first[j+6];
            sum[7] += first[j+7];
            sum[0] += first[j+8];
            sum[1] += first[j+9];
            sum[2] += first[j+10];
            sum[3] += first[j+11];
            sum[4] += first[j+12];
            sum[5] += first[j+13];
            sum[6] += first[j+14];
            sum[7] += first[j+15];
            sum[0] += first[j+16];
            sum[1] += first[j+17];
            sum[2] += first[j+18];
            sum[3] += first[j+19];
            sum[4] += first[j+20];
            sum[5] += first[j+21];
            sum[6] += first[j+22];
            sum[7] += first[j+23];
            sum[0] += first[j+24];
            sum[1] += first[j+25];
            sum[2] += first[j+26];
            sum[3] += first[j+27];
            sum[4] += first[j+28];
            sum[5] += first[j+29];
            sum[6] += first[j+30];
            sum[7] += first[j+31];
        }
        
        for ( ; j < (count - 3); j += 4) {
            sum[0] += first[j+0];
            sum[1] += first[j+1];
            sum[2] += first[j+2];
            sum[3] += first[j+3];
        }

        for (; j < count; ++j) {
            sum[0] += first[j];
        }

        sum[0] += sum[1] + sum[2] + sum[3];
        sum[4] += sum[5] + sum[6] + sum[7];
        sum[0] += sum[4];
        
        return sum[0];
    }
};

/******************************************************************************/
/******************************************************************************/

std::deque<std::string> gLabels;

template <typename T, typename Summer>
void test_accumulate(const T* first, const size_t count, Summer func, const std::string label) {

    start_timer();

    for(int i = 0; i < iterations; ++i) {
        T sum = func( first, count );
        check_sum( sum, label );
    }
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/
/******************************************************************************/

// requires functions converted to classes, but greatly simplifies testing
template<typename Iter, typename T, template<typename, typename > class func >
void TestOneFunc( Iter data, const size_t count, std::string label )
{
    std::string myTypeName( getTypeName<T>() );
    
    const bool isFloat = benchmark::isFloat<T>();
    const bool isSigned = benchmark::isSigned<T>();
    
    test_accumulate( data, SIZE, func<Iter, T>(), label + " to " + myTypeName );
    if (isFloat) {
        if (sizeof(T) < sizeof(float))
            test_accumulate( data, SIZE, func<Iter, float>(), label + " to float" );
        if (sizeof(T) < sizeof(double))
            test_accumulate( data, SIZE, func<Iter, double>(), label + " to double" );
        /*      Nobody is optimizing long double - painful performance.
        if (sizeof(T) < sizeof(long double))
            test_accumulate( data, SIZE, func<Iter, double>(), label + " to long double" );
        */
    } else {
        if (isSigned) {
            if (sizeof(T) < sizeof(int16_t))
                test_accumulate( data, SIZE, func<Iter, int16_t>(), label + " to int16_t" );
            if (sizeof(T) < sizeof(int32_t))
                test_accumulate( data, SIZE, func<Iter, int32_t>(), label + " to int32_t" );
            if (sizeof(T) < sizeof(int64_t))
                test_accumulate( data, SIZE, func<Iter, int64_t>(), label + " to int64_t" );
        } else {
            if (sizeof(T) < sizeof(uint16_t))
                test_accumulate( data, SIZE, func<Iter, uint16_t>(), label + " to uint16_t" );
            if (sizeof(T) < sizeof(uint32_t))
                test_accumulate( data, SIZE, func<Iter, uint32_t>(), label + " to uint32_t" );
            if (sizeof(T) < sizeof(uint64_t))
                test_accumulate( data, SIZE, func<Iter, uint64_t>(), label + " to uint64_t" );
        }
    }
}

/******************************************************************************/
/******************************************************************************/

template<typename T>
void TestOneType()
{
    std::string myTypeName( getTypeName<T>() );
    
    gLabels.clear();
    
    T data[SIZE];

    fill(data, data+SIZE, T(init_value));


    TestOneFunc< const T*, T, accumulate_std >( data, SIZE, myTypeName + " std::accumulate" );
#if _LIBCPP_STD_VER > 14
    TestOneFunc< const T*, T, reduce_add_std >( data, SIZE, myTypeName + " std::reduce" );
#endif
    TestOneFunc< const T*, T, accumulate1 >( data, SIZE, myTypeName + " accumulate1" );
    TestOneFunc< const T*, T, accumulate2 >( data, SIZE, myTypeName + " accumulate2" );
    TestOneFunc< const T*, T, accumulate3 >( data, SIZE, myTypeName + " accumulate3" );
    TestOneFunc< const T*, T, accumulate4 >( data, SIZE, myTypeName + " accumulate4" );
    TestOneFunc< const T*, T, accumulate5 >( data, SIZE, myTypeName + " accumulate5" );
    TestOneFunc< const T*, T, accumulate6 >( data, SIZE, myTypeName + " accumulate6" );
    TestOneFunc< const T*, T, accumulate7 >( data, SIZE, myTypeName + " accumulate7" );
    TestOneFunc< const T*, T, accumulate8 >( data, SIZE, myTypeName + " accumulate8" );
    TestOneFunc< const T*, T, accumulate9 >( data, SIZE, myTypeName + " accumulate9" );
    TestOneFunc< const T*, T, accumulate10 >( data, SIZE, myTypeName + " accumulate10" );
    TestOneFunc< const T*, T, accumulate11 >( data, SIZE, myTypeName + " accumulate11" );
    TestOneFunc< const T*, T, accumulate12 >( data, SIZE, myTypeName + " accumulate12" );
    TestOneFunc< const T*, T, accumulate13 >( data, SIZE, myTypeName + " accumulate13" );
    TestOneFunc< const T*, T, accumulate14 >( data, SIZE, myTypeName + " accumulate14" );
    TestOneFunc< const T*, T, accumulate15 >( data, SIZE, myTypeName + " accumulate15" );
    TestOneFunc< const T*, T, accumulate16 >( data, SIZE, myTypeName + " accumulate16" );
    
    
    std::string temp1( myTypeName + " sum_sequence" );
    summarize(temp1.c_str(), SIZE, iterations, kDontShowGMeans, kDontShowPenalty );

}

/******************************************************************************/
/******************************************************************************/

int main(int argc, char** argv) {

    // output command for documentation:
    int i;
    for (i = 0; i < argc; ++i)
        std::cout << argv[i];
    std::cout << "\n";

    if (argc > 1) iterations = atoi(argv[1]);
    if (argc > 2) init_value = (int32_t) atoi(argv[2]);


    TestOneType<int8_t>();
    TestOneType<uint8_t>();
    TestOneType<int16_t>();
    TestOneType<uint16_t>();
    TestOneType<int32_t>();
    TestOneType<uint32_t>();
    
    iterations /= 4;
    TestOneType<int64_t>();
    TestOneType<uint64_t>();
    TestOneType<float>();
    TestOneType<double>();
//    TestOneType<long double>();   // nobody appears to be generating good code for long double


    return 0;
}

// the end
/******************************************************************************/
/******************************************************************************/
