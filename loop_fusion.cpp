/*
    Copyright 2007-2008 Adobe Systems Incorporated
    Copyright 2018 Chris Cox
    Distributed under the MIT License (see accompanying file LICENSE_1_0_0.txt
    or a copy at http://stlab.adobe.com/licenses.html )


Goal:  Test compiler optimizations related to combining loops.
        aka: loop fusion, loop combining, loop jamming


Assumptions:

    1) The compiler will combine loops without interdependencies,
        when it can improve performance or reduce code size.
        NOTE - cache blocking the loops helps, but fusion helps more.

        i.e. initialize complex values, real then imaginary
        i.e. initialize ARGB buffers one channel at a time

    2) The compiler will combine loops where one result overwrites a previous result,
        when it can improve performance or reduce code size.
        NOTE - cache blocking the loops helps, but fusion helps more.
 
        i.e. init all complex values to zero, then set real part to 1
        i.e. zero ARGB buffer, then set components to fixed values
        i.e. bzero or memset zero a buffer, then set components in the buffer

    3) The compiler will recognize common loop patterns of zero and overwrite,
        and combine loops when it can improve performance or reduce code size.
 
        i.e. bzero or memset zero a buffer, then set all values in the buffer

    4) The compiler will combine loops that require an offset to fuse
        when it can improve performance, or reduce code size.

    5) The compiler will combine or eliminate partial loops that overwrite,
        and combine loops when it can improve performance or reduce code size.

        i.e. zero a buffer, then set values in the second half of the buffer





TODO - rearrange and combine tests by data type, use template for type

TODO - more complex offset loops

TODO - weighted fusion
            multiple choices that could be fused, picking the optimal loops to fuse???

TODO - multiple loop fusion nested

NOTE - different types of loops should be normalized, tested in loop_normalize.cpp

*/

#include "benchmark_stdint.hpp"
#include <cstddef>
#include <cstdio>
#include <time.h>
#include <cstdlib>
#include <iostream>
//#include <cmath>

#if !defined(_WIN32)
#include <strings.h>
#endif

#include "benchmark_results.h"
#include "benchmark_timer.h"

/******************************************************************************/

// this constant may need to be adjusted to give reasonable minimum times
// For best results, times should be about 1.0 seconds for the minimum test run
int iterations = 200;

// 8 million items, intended to be larger than L2 cache on common CPUs
const int SIZE = 8000000;

const int smallSize = 400000;

// initial value for filling our arrays, may be changed from the command line
double init_value = 3.0;

/******************************************************************************/

#include "benchmark_shared_tests.h"

/******************************************************************************/

template <typename T>
inline void check_sum(T result, const char *label) {
  T temp = (T)SIZE * (T)init_value + (T)(SIZE/2);
  if (!tolerance_equal<T>(result,temp))
    std::cout << "test " << label << " failed, got " << result << ", expected " << temp << std::endl;
}

/******************************************************************************/

template <typename T>
inline void check_sum3(T result, const char *label) {
 size_t count = SIZE - (SIZE%3);
  T temp = (T)count * (T)init_value + (T)(count);    // (3*count/3)
  //T temp2 = T( count * (init_value + 1) );
  if (!tolerance_equal<T>(result,temp))
    std::cout << "test " << label << " failed, got " << result << ", expected " << temp << std::endl;
}

/******************************************************************************/

template <typename T>
inline void check_sum4(T result, const char *label) {
 size_t count = SIZE - (SIZE%4);
  T temp = (T)count * (T)init_value + (T)(count);    // (4*count/4)
  if (!tolerance_equal<T>(result,temp))
    std::cout << "test " << label << " failed, got " << result << ", expected " << temp << std::endl;
}

/******************************************************************************/

template <typename T>
inline void check_sum_overwrite(T result, const char *label) {
  T temp = (T)(SIZE/2) * (T)init_value;
  if (!tolerance_equal<T>(result,temp))
    std::cout << "test " << label << " failed, got " << result << ", expected " << temp << std::endl;
}

/******************************************************************************/

template <typename T>
inline void check_sum_overwrite3(T result, const char *label) {
  T temp = (T)(SIZE/3) * (T)(2*init_value);
  if (!tolerance_equal<T>(result,temp))
    std::cout << "test " << label << " failed, got " << result << ", expected " << temp << std::endl;
}

/******************************************************************************/

template <typename T>
inline void check_sum_overwrite4(T result, const char *label) {
  T temp = (T)(SIZE/4) * (T)(2*init_value + 1);
  if (!tolerance_equal<T>(result,temp))
    std::cout << "test " << label << " failed, got " << result << ", expected " << temp << std::endl;
}

/******************************************************************************/

template <typename T>
inline void check_sum_offset(T result, const char *label) {
    T temp = T(smallSize) * T(3*init_value + 4);
    if (!tolerance_equal<T>(result,temp))
        std::cout << "test " << label << " failed, got " << result << ", expected " << temp << std::endl;
}

/******************************************************************************/

template <typename T>
inline void check_sum_replace2(T result, const char *label) {
  T temp = (T)SIZE * (T)init_value;
  if (!tolerance_equal<T>(result,temp))
    std::cout << "test " << label << " failed, got " << result << ", expected " << temp << std::endl;
}

/******************************************************************************/

template <typename T>
inline void check_sum_partial2_replace(T result, const char *label) {
  size_t half = SIZE/2;
  size_t remainder = SIZE - half;
  T temp = T(remainder * (T)init_value);
  if (!tolerance_equal<T>(result,temp))
    std::cout << "test " << label << " failed, got " << result << ", expected " << temp << std::endl;
}

/******************************************************************************/

template <typename T>
inline void check_sum_partial4_replace(T result, const char *label) {
  size_t quarter = SIZE/4;
  size_t remainder = SIZE - 3*quarter;
  T temp = T(quarter * (T)init_value + quarter*T(init_value+1) + remainder*T(init_value+2));
  if (!tolerance_equal<T>(result,temp))
    std::cout << "test " << label << " failed, got " << result << ", expected " << temp << std::endl;
}

/******************************************************************************/
/******************************************************************************/

template <typename T >
void test_loop_2_opt( T* first, int count, const char *label) {
    int i;

    start_timer();

    for(i = 0; i < iterations; ++i) {
        T result = 0;
        int x;

        for (x = 0; x < (count-1); x += 2) {
            first[x+0] = (T)init_value;
            first[x+1] = (T)init_value + (T)1;
            result += first[x+0];
            result += first[x+1];
        }

        check_sum<T>(result,label);
    }

    record_result( timer(), label );
}

/******************************************************************************/

template <typename T >
void test_for_loop_2(T* first, int count, const char *label) {
    int i;

    start_timer();

    for(i = 0; i < iterations; ++i) {
        T result = 0;
        int x;

        for (x = 0; x < (count-1); x += 2) {
            first[x+0] = (T)init_value;
            result += first[x+0];
        }

        for (x = 0; x < (count-1); x += 2) {
            first[x+1] = (T)init_value + (T)1;
            result += first[x+1];
        }

        check_sum<T>(result,label);
    }

    record_result( timer(), label );
}

/******************************************************************************/

template <typename T >
void test_for_loop_blocked2(T* first, int count, const char *label) {
    int i;
    const int block_size = 2 * (4096 / (2*sizeof(T)));

    start_timer();

    for(i = 0; i < iterations; ++i) {
        T result = 0;
        int x, k;
        
        // do cache blocks
        for (k = 0; k < (count-block_size-1); k += block_size) {
            size_t block_end = k + block_size;
            
            for (x = k; x < block_end; x += 2) {
                first[x+0] = (T)init_value;
                result += first[x+0];
            }

            for (x = k; x < block_end; x += 2) {
                first[x+1] = (T)init_value + (T)1;
                result += first[x+1];
            }
        }
        
        // leftovers
        for (x = k; x < (count-1); x += 2) {
            first[x+0] = (T)init_value;
            result += first[x+0];
        }

        for (x = k; x < (count-1); x += 2) {
            first[x+1] = (T)init_value + (T)1;
            result += first[x+1];
        }

        check_sum<T>(result,label);
    }

    record_result( timer(), label );
}

/******************************************************************************/
/******************************************************************************/

template <typename T >
void test_loop_3_opt( T* first, int count, const char *label) {
    int i;

    start_timer();

    for(i = 0; i < iterations; ++i) {
        T result = 0;
        int x;

        for (x = 0; x < (count-2); x += 3) {
            first[x+0] = (T)init_value;
            first[x+1] = (T)init_value + (T)1;
            first[x+2] = (T)init_value + (T)2;
            result += first[x+0];
            result += first[x+1];
            result += first[x+2];
        }

        check_sum3<T>(result,label);
    }

    record_result( timer(), label );
}

/******************************************************************************/

template <typename T >
void test_for_loop_3(T* first, int count, const char *label) {
    int i;

    start_timer();

    for(i = 0; i < iterations; ++i) {
        T result = 0;
        int x;

        for (x = 0; x < (count-2); x += 3) {
            first[x+0] = (T)init_value;
            result += first[x+0];
        }

        for (x = 0; x < (count-2); x += 3) {
            first[x+1] = (T)init_value + (T)1;
            result += first[x+1];
        }

        for (x = 0; x < (count-2); x += 3) {
            first[x+2] = (T)init_value + (T)2;
            result += first[x+2];
        }

        check_sum3<T>(result,label);
    }

    record_result( timer(), label );
}

/******************************************************************************/

template <typename T >
void test_for_loop_blocked3(T* first, int count, const char *label) {
    int i;
    const int block_size = 3 * (4096 / (3*sizeof(T)));

    start_timer();

    for(i = 0; i < iterations; ++i) {
        T result = 0;
        int x, k;
        
        // do cache blocks
        for (k = 0; k < (count-block_size-2); k += block_size ) {
            size_t block_end = k + block_size;
            
            for (x = k; x < block_end; x += 3) {
                first[x+0] = (T)init_value;
                result += first[x+0];
            }

            for (x = k; x < block_end; x += 3) {
                first[x+1] = (T)init_value + (T)1;
                result += first[x+1];
            }

            for (x = k; x < block_end; x += 3) {
                first[x+2] = (T)init_value + (T)2;
                result += first[x+2];
            }
        }
        
        // leftovers
        for (x = k; x < (count-2); x += 3) {
            first[x+0] = (T)init_value;
            result += first[x+0];
        }

        for (x = k; x < (count-2); x += 3) {
            first[x+1] = (T)init_value + (T)1;
            result += first[x+1];
        }

        for (x = k; x < (count-2); x += 3) {
            first[x+2] = (T)init_value + (T)2;
            result += first[x+2];
        }

        check_sum3<T>(result,label);
    }

    record_result( timer(), label );
}

/******************************************************************************/
/******************************************************************************/

template <typename T >
void test_loop_4_opt( T* first, int count, const char *label) {
    int i;

    start_timer();

    for(i = 0; i < iterations; ++i) {
        T result = 0;
        int x;

        for (x = 0; x < (count-3); x += 4) {
            first[x+0] = (T)init_value;
            first[x+1] = (T)init_value + (T)1;
            first[x+2] = (T)init_value + (T)2;
            first[x+3] = (T)init_value + (T)1;
            result += first[x+0];
            result += first[x+1];
            result += first[x+2];
            result += first[x+3];
        }

        check_sum4<T>(result,label);
    }

    record_result( timer(), label );
}

/******************************************************************************/

template <typename T >
void test_for_loop_4(T* first, int count, const char *label) {
    int i;

    start_timer();

    for(i = 0; i < iterations; ++i) {
        T result = 0;
        int x;

        for (x = 0; x < (count-3); x += 4) {
            first[x+0] = (T)init_value;
            result += first[x+0];
        }

        for (x = 0; x < (count-3); x += 4) {
            first[x+1] = (T)init_value + (T)1;
            result += first[x+1];
        }

        for (x = 0; x < (count-3); x += 4) {
            first[x+2] = (T)init_value + (T)2;
            result += first[x+2];
        }

        for (x = 0; x < (count-3); x += 4) {
            first[x+3] = (T)init_value + (T)1;
            result += first[x+3];
        }

        check_sum4<T>(result,label);
    }

    record_result( timer(), label );
}

/******************************************************************************/

template <typename T >
void test_for_loop_blocked4(T* first, int count, const char *label) {
    int i;
    const int block_size = 4 * (4096 / (4*sizeof(T)));

    start_timer();

    for(i = 0; i < iterations; ++i) {
        T result = 0;
        int x, k;
        
        // do cache blocks
        for (k = 0; k < (count-block_size-3); k += block_size) {
            size_t block_end = k + block_size;
            
            for (x = k; x < block_end; x += 4) {
                first[x+0] = (T)init_value;
                result += first[x+0];
            }

            for (x = k; x < block_end; x += 4) {
                first[x+1] = (T)init_value + (T)1;
                result += first[x+1];
            }

            for (x = k; x < block_end; x += 4) {
                first[x+2] = (T)init_value + (T)2;
                result += first[x+2];
            }

            for (x = k; x < block_end; x += 4) {
                first[x+3] = (T)init_value + (T)1;
                result += first[x+3];
            }
        }
        
        // leftovers
        for (x = k; x < (count-3); x += 4) {
            first[x+0] = (T)init_value;
            result += first[x+0];
        }

        for (x = k; x < (count-3); x += 4) {
            first[x+1] = (T)init_value + (T)1;
            result += first[x+1];
        }

        for (x = k; x < (count-3); x += 4) {
            first[x+2] = (T)init_value + (T)2;
            result += first[x+2];
        }

        for (x = k; x < (count-3); x += 4) {
            first[x+3] = (T)init_value + (T)1;
            result += first[x+3];
        }

        check_sum4<T>(result,label);
    }

    record_result( timer(), label );
}

/******************************************************************************/
/******************************************************************************/

template <typename T >
void test_loop_overwrite2_opt( T* first, int count, const char *label) {
    int i, x;
    T result = 0;

    start_timer();

    for(i = 0; i < iterations; ++i) {
        int x;

        for (x = 0; x < (count-1); x += 2) {
            first[x+0] = (T)0;
            first[x+1] = (T)init_value;
        }
    }

    record_result( timer(), label );

    for (x = 0; x < count; ++x) {
        result += first[x];
    }

    check_sum_overwrite<T>(result,label);
}

/******************************************************************************/

template <typename T >
void test_for_loop_overwrite2(T* first, int count, const char *label) {
    int i, x;
    T result = 0;

    start_timer();

    for(i = 0; i < iterations; ++i) {

        for (x = 0; x < count; ++x) {
            first[x] = (T)0;
        }

        for (x = 0; x < (count-1); x += 2) {
            first[x+1] = (T)init_value;
        }
    }

    record_result( timer(), label );

    for (x = 0; x < count; ++x) {
        result += first[x];
    }

    check_sum_overwrite<T>(result,label);
}

/******************************************************************************/

#if !defined(_WIN32)
template <typename T >
void test_for_loop_bzero_overwrite2(T* first, int count, const char *label) {
    int i, x;
    T result = 0;

    start_timer();

    for(i = 0; i < iterations; ++i) {

        bzero( first, count*sizeof(T) );
        
        for (x = 0; x < (count-1); x += 2) {
            first[x+1] = (T)init_value;
        }
    }

    record_result( timer(), label );

    for (x = 0; x < count; ++x) {
        result += first[x];
    }

    check_sum_overwrite<T>(result,label);
}
#endif

/******************************************************************************/

template <typename T >
void test_for_loop_memset_overwrite2(T* first, int count, const char *label) {
    int i, x;
    T result = 0;

    start_timer();

    for(i = 0; i < iterations; ++i) {

        memset( first, 0, count*sizeof(T) );
        
        for (x = 0; x < (count-1); x += 2) {
            first[x+1] = (T)init_value;
        }
    }

    record_result( timer(), label );

    for (x = 0; x < count; ++x) {
        result += first[x];
    }

    check_sum_overwrite<T>(result,label);
}

/******************************************************************************/

template <typename T >
void test_for_loop_blocked_overwrite2(T* first, int count, const char *label) {
    T result = 0;
    int i, x;
    const int block_size = 4096 / (2*sizeof(T));

    start_timer();

    for(i = 0; i < iterations; ++i) {
        int k;
        
        // do cache blocks
        for (k = 0; k < (count-block_size+1); k += block_size) {
            size_t block_end = k + block_size - 1;
            
            for (x = k; x < (k + block_size); ++x ) {
                first[x+0] = (T)0;
            }

            for (x = k; x < block_end; x += 2) {
                first[x+1] = (T)init_value;
            }
        }
        
        // leftovers
        for (x = k; x < count; ++x) {
            first[x] = (T)0;
        }

        for (x = k; x < (count-1); x += 2) {
            first[x+1] = (T)init_value;
        }
    }

    record_result( timer(), label );

    for (x = 0; x < count; ++x) {
        result += first[x];
    }

    check_sum_overwrite<T>(result,label);
}

/******************************************************************************/
/******************************************************************************/

template <typename T >
void test_loop_overwrite3_opt( T* first, int count, const char *label) {
    int i, x;
    T result = 0;

    start_timer();

    for(i = 0; i < iterations; ++i) {

        for (x = 0; x < (count-2); x += 3) {
            first[x+0] = (T)0;
            first[x+1] = (T)init_value;
            first[x+2] = (T)init_value;
        }

        for (; x < count; ++x) {
            first[x] = (T)0;
        }
    }

    record_result( timer(), label );

    for (x = 0; x < count; ++x) {
        result += first[x];
    }

    check_sum_overwrite3<T>(result,label);
}

/******************************************************************************/

template <typename T >
void test_for_loop_overwrite3(T* first, int count, const char *label) {
    int i, x;
    T result = 0;

    start_timer();

    for(i = 0; i < iterations; ++i) {

        for (x = 0; x < count; ++x) {
            first[x] = (T)0;
        }

        for (x = 0; x < (count-2); x += 3) {
            first[x+1] = (T)init_value;
        }

        for (x = 0; x < (count-2); x += 3) {
            first[x+2] = (T)init_value;
        }
    }

    record_result( timer(), label );

    for (x = 0; x < count; ++x) {
        result += first[x];
    }

    check_sum_overwrite3<T>(result,label);
}

/******************************************************************************/

#if !defined(_WIN32)
template <typename T >
void test_for_loop_bzero_overwrite3(T* first, int count, const char *label) {
    int i, x;
    T result = 0;

    start_timer();

    for(i = 0; i < iterations; ++i) {

        bzero( first, count*sizeof(T) );

        for (x = 0; x < (count-2); x += 3) {
            first[x+1] = (T)init_value;
        }

        for (x = 0; x < (count-2); x += 3) {
            first[x+2] = (T)init_value;
        }
    }

    record_result( timer(), label );

    for (x = 0; x < count; ++x) {
        result += first[x];
    }

    check_sum_overwrite3<T>(result,label);
}
#endif

/******************************************************************************/

template <typename T >
void test_for_loop_memset_overwrite3(T* first, int count, const char *label) {
    int i, x;
    T result = 0;

    start_timer();

    for(i = 0; i < iterations; ++i) {

        memset( first, 0, count*sizeof(T) );

        for (x = 0; x < (count-2); x += 3) {
            first[x+1] = (T)init_value;
        }

        for (x = 0; x < (count-2); x += 3) {
            first[x+2] = (T)init_value;
        }
    }

    record_result( timer(), label );

    for (x = 0; x < count; ++x) {
        result += first[x];
    }

    check_sum_overwrite3<T>(result,label);
}

/******************************************************************************/

template <typename T >
void test_for_loop_blocked_overwrite3(T* first, int count, const char *label) {
    T result = 0;
    int i, x;
    const size_t block_size = 3 * (4096 / (3*sizeof(T)));

    start_timer();

    for(i = 0; i < iterations; ++i) {
        int k;
        
        // do cache blocks
        for (k = 0; k < (count-block_size-2);) {
            size_t block_end = k + block_size;
            
            for (x = k; x < block_end; ++x ) {
                first[x+0] = (T)0;
            }

            for (x = k; x < block_end; x += 3) {
                first[x+1] = (T)init_value;
            }

            for (x = k; x < block_end; x += 3) {
                first[x+2] = (T)init_value;
            }
            
            k = block_end;
        }
        
        // leftovers
        for (x = k; x < count; ++x) {
            first[x] = (T)0;
        }

        for (x = k; x < (count-2); x += 3) {
            first[x+1] = (T)init_value;
        }

        for (x = k; x < (count-2); x += 3) {
            first[x+2] = (T)init_value;
        }
    }

    record_result( timer(), label );

    for (x = 0; x < count; ++x) {
        result += first[x];
    }

    check_sum_overwrite3<T>(result,label);
}

/******************************************************************************/
/******************************************************************************/

template <typename T >
void test_loop_overwrite4_opt( T* first, int count, const char *label) {
    int i, x;
    T result = 0;

    start_timer();

    for(i = 0; i < iterations; ++i) {

        for (x = 0; x < (count-3); x += 4) {
            first[x+0] = (T)0;
            first[x+1] = (T)init_value;
            first[x+2] = (T)init_value;
            first[x+3] = (T)1;
        }
    
    }

    record_result( timer(), label );

    for (x = 0; x < count; ++x) {
        result += first[x];
    }

    check_sum_overwrite4<T>(result,label);
}

/******************************************************************************/

template <typename T >
void test_for_loop_overwrite4(T* first, int count, const char *label) {
    int i, x;
    T result = 0;

    start_timer();

    for(i = 0; i < iterations; ++i) {

        for (x = 0; x < count; ++x) {
            first[x] = (T)0;
        }

        for (x = 0; x < (count-3); x += 4) {
            first[x+1] = (T)init_value;
        }

        for (x = 0; x < (count-3); x += 4) {
            first[x+2] = (T)init_value;
        }

        for (x = 0; x < (count-3); x += 4) {
            first[x+3] = (T)1;
        }
    
    }

    record_result( timer(), label );

    for (x = 0; x < count; ++x) {
        result += first[x];
    }

    check_sum_overwrite4<T>(result,label);
}

/******************************************************************************/

#if !defined(_WIN32)
template <typename T >
void test_for_loop_bzero_overwrite4(T* first, int count, const char *label) {
    int i, x;
    T result = 0;

    start_timer();

    for(i = 0; i < iterations; ++i) {

        bzero(first, count*sizeof(T) );

        for (x = 0; x < (count-3); x += 4) {
            first[x+1] = (T)init_value;
        }

        for (x = 0; x < (count-3); x += 4) {
            first[x+2] = (T)init_value;
        }

        for (x = 0; x < (count-3); x += 4) {
            first[x+3] = (T)1;
        }
    
    }

    record_result( timer(), label );

    for (x = 0; x < count; ++x) {
        result += first[x];
    }

    check_sum_overwrite4<T>(result,label);
}
#endif

/******************************************************************************/

template <typename T >
void test_for_loop_memset_overwrite4(T* first, int count, const char *label) {
    int i, x;
    T result = 0;

    start_timer();

    for(i = 0; i < iterations; ++i) {

        memset(first, 0, count*sizeof(T) );

        for (x = 0; x < (count-3); x += 4) {
            first[x+1] = (T)init_value;
        }

        for (x = 0; x < (count-3); x += 4) {
            first[x+2] = (T)init_value;
        }

        for (x = 0; x < (count-3); x += 4) {
            first[x+3] = (T)1;
        }
    
    }

    record_result( timer(), label );

    for (x = 0; x < count; ++x) {
        result += first[x];
    }

    check_sum_overwrite4<T>(result,label);
}


/******************************************************************************/

template <typename T >
void test_for_loop_blocked_overwrite4(T* first, int count, const char *label) {
    T result = 0;
    int i, x;
    const int block_size = 4096 / (4*sizeof(T));

    start_timer();

    for(i = 0; i < iterations; ++i) {
        int k;
        
        // do cache blocks
        for (k = 0; k < (count-block_size+1); k += block_size) {
            size_t block_end = k + block_size - 3;
            
            for (x = k; x < (k + block_size); ++x ) {
                first[x+0] = (T)0;
            }

            for (x = k; x < block_end; x += 4) {
                first[x+1] = (T)init_value;
            }

            for (x = k; x < block_end; x += 4) {
                first[x+2] = (T)init_value;
            }

            for (x = k; x < block_end; x += 4) {
                first[x+3] = (T)1;
            }
        }
        
        // leftovers
        for (x = k; x < count; ++x) {
            first[x] = (T)0;
        }

        for (x = k; x < (count-3); x += 4) {
            first[x+1] = (T)init_value;
        }

        for (x = k; x < (count-3); x += 4) {
            first[x+2] = (T)init_value;
        }

        for (x = k; x < (count-3); x += 4) {
            first[x+3] = (T)1;
        }
    }

    record_result( timer(), label );

    for (x = 0; x < count; ++x) {
        result += first[x];
    }

    check_sum_overwrite4<T>(result,label);
}

/******************************************************************************/
/******************************************************************************/

template <typename T >
void test_loop_replace_opt( T* first, int count, const char *label) {
    int i, x;
    T result = 0;

    start_timer();

    for(i = 0; i < iterations; ++i) {

        for (x = 0; x < count; ++x) {
            first[x] = (T)init_value;
        }
    
    }

    record_result( timer(), label );

    for (x = 0; x < count; ++x) {
        result += first[x];
    }

    check_sum_replace2<T>(result,label);
}

/******************************************************************************/

template <typename T >
void test_for_loop_replace2( T* first, int count, const char *label) {
    int i, x;
    T result = 0;

    start_timer();

    for(i = 0; i < iterations; ++i) {

        for (x = 0; x < count; ++x) {
            first[x] = (T)0;
        }

        for (x = 0; x < count; ++x) {
            first[x] = (T)init_value;
        }
    
    }

    record_result( timer(), label );

    for (x = 0; x < count; ++x) {
        result += first[x];
    }

    check_sum_replace2<T>(result,label);
}

/******************************************************************************/

template <typename T >
void test_for_loop_blocked_replace2( T* first, int count, const char *label) {
    int i, x;
    T result = 0;
    const int block_size = 4096 / (2*sizeof(T));

    start_timer();

    for(i = 0; i < iterations; ++i) {
        int k;
        
        // do cache blocks
        for (k = 0; k < (count-block_size); k += block_size) {
            size_t block_end = k + block_size;

            for (x = k; x < block_end; ++x) {
                first[x] = (T)0;
            }

            for (x = k; x < block_end; ++x) {
                first[x] = (T)init_value;
            }
        }
        
        // leftovers
        for (x = k; x < count; ++x) {
            first[x] = (T)0;
        }

        for (x = k; x < count; ++x) {
            first[x] = (T)init_value;
        }

    }

    record_result( timer(), label );

    for (x = 0; x < count; ++x) {
        result += first[x];
    }

    check_sum_replace2<T>(result,label);
}

/******************************************************************************/

#if !defined(_WIN32)
template <typename T >
void test_for_loop_bzero_replace2( T* first, int count, const char *label) {
    int i, x;
    T result = 0;

    start_timer();

    for(i = 0; i < iterations; ++i) {

        bzero( first, count*sizeof(T) );

        for (x = 0; x < count; ++x) {
            first[x] = (T)init_value;
        }
    
    }

    record_result( timer(), label );

    for (x = 0; x < count; ++x) {
        result += first[x];
    }

    check_sum_replace2<T>(result,label);
}
#endif

/******************************************************************************/

template <typename T >
void test_for_loop_memset_replace2( T* first, int count, const char *label) {
    int i, x;
    T result = 0;

    start_timer();

    for(i = 0; i < iterations; ++i) {

        memset( first, 0, count*sizeof(T) );

        for (x = 0; x < count; ++x) {
            first[x] = (T)init_value;
        }
    }

    record_result( timer(), label );

    for (x = 0; x < count; ++x) {
        result += first[x];
    }

    check_sum_replace2<T>(result,label);
}

/******************************************************************************/

template <typename T >
void test_for_loop_replace3( T* first, int count, const char *label) {
    int i, x;
    T result = 0;

    start_timer();

    for(i = 0; i < iterations; ++i) {

        for (x = 0; x < count; ++x) {
            first[x] = (T)0;
        }

        for (x = 0; x < count; ++x) {
            first[x] = (T)1;
        }

        for (x = 0; x < count; ++x) {
            first[x] = (T)init_value;
        }
    
    }

    record_result( timer(), label );

    for (x = 0; x < count; ++x) {
        result += first[x];
    }

    check_sum_replace2<T>(result,label);
}

/******************************************************************************/

template <typename T >
void test_for_loop_blocked_replace3( T* first, int count, const char *label) {
    int i, x;
    T result = 0;
    const int block_size = 4096 / (2*sizeof(T));

    start_timer();

    for(i = 0; i < iterations; ++i) {
        int k;
        
        // do cache blocks
        for (k = 0; k < (count-block_size); k += block_size) {
            size_t block_end = k + block_size;

            for (x = k; x < block_end; ++x) {
                first[x] = (T)0;
            }

            for (x = k; x < block_end; ++x) {
                first[x] = (T)1;
            }

            for (x = k; x < block_end; ++x) {
                first[x] = (T)init_value;
            }
        }
        
        // leftovers
        for (x = k; x < count; ++x) {
            first[x] = (T)0;
        }

        for (x = k; x < count; ++x) {
            first[x] = (T)1;
        }

        for (x = k; x < count; ++x) {
            first[x] = (T)init_value;
        }

    }

    record_result( timer(), label );

    for (x = 0; x < count; ++x) {
        result += first[x];
    }

    check_sum_replace2<T>(result,label);
}

/******************************************************************************/

#if !defined(_WIN32)
template <typename T >
void test_for_loop_bzero_replace3( T* first, int count, const char *label) {
    int i, x;
    T result = 0;

    start_timer();

    for(i = 0; i < iterations; ++i) {

        bzero( first, count*sizeof(T) );

        for (x = 0; x < count; ++x) {
            first[x] = (T)1;
        }

        for (x = 0; x < count; ++x) {
            first[x] = (T)init_value;
        }
    
    }

    record_result( timer(), label );

    for (x = 0; x < count; ++x) {
        result += first[x];
    }

    check_sum_replace2<T>(result,label);
}
#endif

/******************************************************************************/

template <typename T >
void test_for_loop_memset_replace3( T* first, int count, const char *label) {
    int i, x;
    T result = 0;

    start_timer();

    for(i = 0; i < iterations; ++i) {

        memset( first, 0, count*sizeof(T) );

        for (x = 0; x < count; ++x) {
            first[x] = (T)1;
        }

        for (x = 0; x < count; ++x) {
            first[x] = (T)init_value;
        }
    }

    record_result( timer(), label );

    for (x = 0; x < count; ++x) {
        result += first[x];
    }

    check_sum_replace2<T>(result,label);
}

/******************************************************************************/

template <typename T >
void test_for_loop_replace4( T* first, int count, const char *label) {
    int i, x;
    T result = 0;

    start_timer();

    for(i = 0; i < iterations; ++i) {

        for (x = 0; x < count; ++x) {
            first[x] = (T)0;
        }

        for (x = 0; x < count; ++x) {
            first[x] = (T)11;
        }

        for (x = 0; x < count; ++x) {
            first[x] = (T)99;
        }

        for (x = 0; x < count; ++x) {
            first[x] = (T)init_value;
        }
    
    }

    record_result( timer(), label );

    for (x = 0; x < count; ++x) {
        result += first[x];
    }

    check_sum_replace2<T>(result,label);
}

/******************************************************************************/

template <typename T >
void test_for_loop_blocked_replace4( T* first, int count, const char *label) {
    int i, x;
    T result = 0;
    const int block_size = 4096 / (2*sizeof(T));

    start_timer();

    for(i = 0; i < iterations; ++i) {
        int k;
        
        // do cache blocks
        for (k = 0; k < (count-block_size); k += block_size) {
            size_t block_end = k + block_size;

            for (x = k; x < block_end; ++x) {
                first[x] = (T)0;
            }

            for (x = k; x < block_end; ++x) {
                first[x] = (T)11;
            }

            for (x = k; x < block_end; ++x) {
                first[x] = (T)99;
            }

            for (x = k; x < block_end; ++x) {
                first[x] = (T)init_value;
            }
        }
        
        // leftovers
        for (x = k; x < count; ++x) {
            first[x] = (T)0;
        }

        for (x = k; x < count; ++x) {
            first[x] = (T)11;
        }

        for (x = k; x < count; ++x) {
            first[x] = (T)99;
        }

        for (x = k; x < count; ++x) {
            first[x] = (T)init_value;
        }

    }

    record_result( timer(), label );

    for (x = 0; x < count; ++x) {
        result += first[x];
    }

    check_sum_replace2<T>(result,label);
}

/******************************************************************************/

#if !defined(_WIN32)
template <typename T >
void test_for_loop_bzero_replace4( T* first, int count, const char *label) {
    int i, x;
    T result = 0;

    start_timer();

    for(i = 0; i < iterations; ++i) {

        bzero( first, count*sizeof(T) );

        for (x = 0; x < count; ++x) {
            first[x] = (T)11;
        }

        for (x = 0; x < count; ++x) {
            first[x] = (T)99;
        }

        for (x = 0; x < count; ++x) {
            first[x] = (T)init_value;
        }
    
    }

    record_result( timer(), label );

    for (x = 0; x < count; ++x) {
        result += first[x];
    }

    check_sum_replace2<T>(result,label);
}
#endif

/******************************************************************************/

template <typename T >
void test_for_loop_memset_replace4( T* first, int count, const char *label) {
    int i, x;
    T result = 0;

    start_timer();

    for(i = 0; i < iterations; ++i) {

        memset( first, 0, count*sizeof(T) );

        for (x = 0; x < count; ++x) {
            first[x] = (T)11;
        }

        for (x = 0; x < count; ++x) {
            first[x] = (T)99;
        }

        for (x = 0; x < count; ++x) {
            first[x] = (T)init_value;
        }
    }

    record_result( timer(), label );

    for (x = 0; x < count; ++x) {
        result += first[x];
    }

    check_sum_replace2<T>(result,label);
}

/******************************************************************************/
/******************************************************************************/

template <typename T >
void test_loop_offset2(T* first, T *second, T * third, int count, const char *label) {
    int i, x;
    T result = 0;

    start_timer();

    for(i = 0; i < iterations; ++i) {

        for (x = 0; x < count; ++x)
            first[x] = third[x] + 1;

        for (x = 0; x < (count-1); ++x)
            second[x] = first[x+1] + third[x] + 2;
        
        second[count-1] = 2*third[0] + 3;
        
    }

    record_result( timer(), label );

    for (x = 0; x < count; ++x) {
        result += first[x] + second[x];
    }

    check_sum_offset<T>(result,label);
}

/******************************************************************************/

template <typename T >
void test_loop_offset2_opt(T* first, T *second, T * third, int count, const char *label) {
    int i, x;
    T result = 0;

    start_timer();

    for(i = 0; i < iterations; ++i) {
    
        first[0] = third[0] + 1;
        T thirdX = third[0];
        
        for (x = 0; x < (count-1); ++x) {
            T thirdX1 = third[x+1];
            T temp = thirdX1 + 1;
            first[x+1] = temp;
            second[x+0] = temp + thirdX + 2;
            thirdX = thirdX1;
        }
        
        second[count-1] = 2*third[0] + 3;
        
    }

    record_result( timer(), label );

    for (x = 0; x < count; ++x) {
        result += first[x] + second[x];
    }

    check_sum_offset<T>(result,label);
}

/******************************************************************************/
/******************************************************************************/

template <typename T >
void test_loop_partial2_replace_opt( T* first, int count, const char *label) {
    int i, x;
    T result = 0;

    start_timer();

    for(i = 0; i < iterations; ++i) {

        for (x = 0; x < count/2; ++x) {
            first[x] = (T)0;
        }
        for (x=(count/2); x < count; ++x) {
            first[x] = (T)init_value;
        }
    
    }

    record_result( timer(), label );

    for (x = 0; x < count; ++x) {
        result += first[x];
    }

    check_sum_partial2_replace<T>(result,label);
}

/******************************************************************************/

template <typename T >
void test_for_loop_partial2_replace( T* first, int count, const char *label) {
    int i, x;
    T result = 0;

    start_timer();

    for(i = 0; i < iterations; ++i) {

        for (x = 0; x < count; ++x) {
            first[x] = (T)0;
        }

        for (x = (count/2); x < count; ++x) {
            first[x] = (T)init_value;
        }
    
    }

    record_result( timer(), label );

    for (x = 0; x < count; ++x) {
        result += first[x];
    }

    check_sum_partial2_replace<T>(result,label);
}

/******************************************************************************/

#if !defined(_WIN32)
template <typename T >
void test_for_loop_bzero_partial2_replace( T* first, int count, const char *label) {
    int i, x;
    T result = 0;

    start_timer();

    for(i = 0; i < iterations; ++i) {

        bzero( first, count*sizeof(T) );

        for (x = (count/2); x < count; ++x) {
            first[x] = (T)init_value;
        }
    
    }

    record_result( timer(), label );

    for (x = 0; x < count; ++x) {
        result += first[x];
    }

    check_sum_partial2_replace<T>(result,label);
}
#endif

/******************************************************************************/

template <typename T >
void test_for_loop_memset_partial2_replace( T* first, int count, const char *label) {
    int i, x;
    T result = 0;

    start_timer();

    for(i = 0; i < iterations; ++i) {

        memset( first, 0, count*sizeof(T) );

        for (x = (count/2); x < count; ++x) {
            first[x] = (T)init_value;
        }
    }

    record_result( timer(), label );

    for (x = 0; x < count; ++x) {
        result += first[x];
    }

    check_sum_partial2_replace<T>(result,label);
}
/******************************************************************************/

template <typename T >
void test_loop_partial4_replace_opt( T* first, int count, const char *label) {
    int i, x;
    T result = 0;
    int quarter = count/4;

    start_timer();

    for(i = 0; i < iterations; ++i) {

        for (x = 0; x < quarter; ++x) {
            first[x] = (T)0;
        }
        for (x = quarter; x < (2*quarter); ++x) {
            first[x] = (T)init_value;
        }
        for (x = (2*quarter); x < (3*quarter); ++x) {
            first[x] = (T)init_value + 1;
        }
        for (x = (3*quarter); x < count; ++x) {
            first[x] = (T)init_value + 2;
        }
    
    }

    record_result( timer(), label );

    for (x = 0; x < count; ++x) {
        result += first[x];
    }

    check_sum_partial4_replace<T>(result,label);
}

/******************************************************************************/

template <typename T >
void test_for_loop_partial4_replace( T* first, int count, const char *label) {
    int i, x;
    T result = 0;
    int quarter = count/4;

    start_timer();

    for(i = 0; i < iterations; ++i) {

        for (x = 0; x < count; ++x) {
            first[x] = (T)0;
        }

        for (x = quarter; x < (2*quarter); ++x) {
            first[x] = (T)init_value;
        }

        for (x = (2*quarter); x < (3*quarter); ++x) {
            first[x] = (T)init_value + 1;
        }

        for (x = (3*quarter); x < count; ++x) {
            first[x] = (T)init_value + 2;
        }
    
    }

    record_result( timer(), label );

    for (x = 0; x < count; ++x) {
        result += first[x];
    }

    check_sum_partial4_replace<T>(result,label);
}

/******************************************************************************/

// evil, but should still be optimized
template <typename T >
void test_for_loop_partial4_replaceA( T* first, int count, const char *label) {
    int i, x;
    T result = 0;
    int quarter = count/4;

    start_timer();

    for(i = 0; i < iterations; ++i) {

        for (x = 0; x < count; ++x) {
            first[x] = (T)0;
        }

        for (x = quarter; x < count; ++x) {
            first[x] = (T)init_value;
        }

        for (x = (2*quarter); x < count; ++x) {
            first[x] = (T)init_value + 1;
        }

        for (x = (3*quarter); x < count; ++x) {
            first[x] = (T)init_value + 2;
        }
    
    }

    record_result( timer(), label );

    for (x = 0; x < count; ++x) {
        result += first[x];
    }

    check_sum_partial4_replace<T>(result,label);
}

/******************************************************************************/

#if !defined(_WIN32)
template <typename T >
void test_for_loop_bzero_partial4_replace( T* first, int count, const char *label) {
    int i, x;
    T result = 0;
    int quarter = count/4;

    start_timer();

    for(i = 0; i < iterations; ++i) {

        bzero( first, count*sizeof(T) );

        for (x = quarter; x < (2*quarter); ++x) {
            first[x] = (T)init_value;
        }

        for (x = (2*quarter); x < (3*quarter); ++x) {
            first[x] = (T)init_value + 1;
        }

        for (x = (3*quarter); x < count; ++x) {
            first[x] = (T)init_value + 2;
        }
    
    }

    record_result( timer(), label );

    for (x = 0; x < count; ++x) {
        result += first[x];
    }

    check_sum_partial4_replace<T>(result,label);
}
#endif

/******************************************************************************/

template <typename T >
void test_for_loop_memset_partial4_replace( T* first, int count, const char *label) {
    int i, x;
    T result = 0;
    int quarter = count/4;

    start_timer();

    for(i = 0; i < iterations; ++i) {

        memset( first, 0, count*sizeof(T) );

        for (x = quarter; x < (2*quarter); ++x) {
            first[x] = (T)init_value;
        }

        for (x = (2*quarter); x < (3*quarter); ++x) {
            first[x] = (T)init_value + 1;
        }

        for (x = (3*quarter); x < count; ++x) {
            first[x] = (T)init_value + 2;
        }
    }

    record_result( timer(), label );

    for (x = 0; x < count; ++x) {
        result += first[x];
    }

    check_sum_partial4_replace<T>(result,label);
}

/******************************************************************************/
/******************************************************************************/

// our global arrays of numbers to be operated upon

double dataDouble[SIZE];
double dataDoubleB[smallSize];
double dataDoubleC[smallSize];

int32_t data32[SIZE];
int32_t data32B[smallSize];
int32_t data32C[smallSize];

uint8_t data8[SIZE];
uint8_t data8B[smallSize];
uint8_t data8C[smallSize];

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


// uint8_t
    test_loop_2_opt( data8, SIZE, "uint8_t loop fusion 2way indep optimal");
    test_for_loop_blocked2( data8, SIZE, "uint8_t for loop fusion 2way indep blocked");
    test_for_loop_2( data8, SIZE, "uint8_t for loop fusion 2way indep");
    
    summarize("uint8_t loop fusion 2way indep", SIZE, iterations, kDontShowGMeans, kDontShowPenalty );


    test_loop_3_opt( data8, SIZE, "uint8_t loop fusion 3way indep optimal");
    test_for_loop_blocked3( data8, SIZE, "uint8_t for loop fusion 3way indep blocked");
    test_for_loop_3( data8, SIZE, "uint8_t for loop fusion 3way indep");
    
    summarize("uint8_t loop fusion 3way indep", SIZE, iterations, kDontShowGMeans, kDontShowPenalty );


    test_loop_4_opt( data8, SIZE, "uint8_t loop fusion 4way indep optimal");
    test_for_loop_blocked4( data8, SIZE, "uint8_t for loop fusion 4way indep blocked");
    test_for_loop_4( data8, SIZE, "uint8_t for loop fusion 4way indep");
    
    summarize("uint8_t loop fusion 4way indep", SIZE, iterations, kDontShowGMeans, kDontShowPenalty );


// int32_t
    test_loop_2_opt( data32, SIZE, "int32_t loop fusion 2way indep optimal");
    test_for_loop_blocked2( data32, SIZE, "int32_t for loop fusion 2way indep blocked");
    test_for_loop_2( data32, SIZE, "int32_t for loop fusion 2way indep");
    
    summarize("int32_t loop fusion 2way indep", SIZE, iterations, kDontShowGMeans, kDontShowPenalty );


    test_loop_3_opt( data32, SIZE, "int32_t loop fusion 3way indep optimal");
    test_for_loop_blocked3( data32, SIZE, "int32_t for loop fusion 3way indep blocked");
    test_for_loop_3( data32, SIZE, "int32_t for loop fusion 3way indep");
    
    summarize("int32_t loop fusion 3way indep", SIZE, iterations, kDontShowGMeans, kDontShowPenalty );

    test_loop_4_opt( data32, SIZE, "int32_t loop fusion 4way indep optimal");
    test_for_loop_blocked4( data32, SIZE, "int32_t for loop fusion 4way indep blocked");
    test_for_loop_4( data32, SIZE, "int32_t for loop fusion 4way indep");
    
    summarize("int32_t loop fusion 4way indep", SIZE, iterations, kDontShowGMeans, kDontShowPenalty );


// double
    test_loop_2_opt( dataDouble, SIZE, "double loop fusion 2way indep optimal");
    test_for_loop_blocked2( dataDouble, SIZE, "double for loop fusion 2way indep blocked");
    test_for_loop_2( dataDouble, SIZE, "double for loop fusion 2way indep");
    
    summarize("double loop fusion 2way indep", SIZE, iterations, kDontShowGMeans, kDontShowPenalty );
    
    
    test_loop_3_opt( dataDouble, SIZE, "double loop fusion 3way indep optimal");
    test_for_loop_blocked3( dataDouble, SIZE, "double for loop fusion 3way indep blocked");
    test_for_loop_3( dataDouble, SIZE, "double for loop fusion 3way indep");
    
    summarize("double loop fusion 3way indep", SIZE, iterations, kDontShowGMeans, kDontShowPenalty );
    
    
    test_loop_4_opt( dataDouble, SIZE, "double loop fusion 4way indep optimal");
    test_for_loop_blocked4( dataDouble, SIZE, "double for loop fusion 4way indep blocked");
    test_for_loop_4( dataDouble, SIZE, "double for loop fusion 4way indep");
    
    summarize("double loop fusion 4way indep", SIZE, iterations, kDontShowGMeans, kDontShowPenalty );



// uint8_t
    test_loop_overwrite2_opt( data8, SIZE, "uint8_t loop fusion 2way overwrite optimal");
    test_for_loop_blocked_overwrite2( data8, SIZE, "uint8_t for loop fusion 2way overwrite blocked");
#if !defined(_WIN32)
    test_for_loop_bzero_overwrite2( data8, SIZE, "uint8_t for loop fusion 2way overwrite bzero");
#endif
    test_for_loop_memset_overwrite2( data8, SIZE, "uint8_t for loop fusion 2way overwrite memset");
    test_for_loop_overwrite2( data8, SIZE, "uint8_t for loop fusion 2way overwrite");
    
    summarize("uint8_t loop fusion 2way overwrite", SIZE, iterations, kDontShowGMeans, kDontShowPenalty );


    test_loop_overwrite3_opt( data8, SIZE, "uint8_t loop fusion 3way overwrite optimal");
    test_for_loop_blocked_overwrite3( data8, SIZE, "uint8_t for loop fusion 3way overwrite blocked");
#if !defined(_WIN32)
    test_for_loop_bzero_overwrite3( data8, SIZE, "uint8_t for loop fusion 3way overwrite bzero");
#endif
    test_for_loop_memset_overwrite3( data8, SIZE, "uint8_t for loop fusion 3way overwrite memset");
    test_for_loop_overwrite3( data8, SIZE, "uint8_t for loop fusion 3way overwrite");
    
    summarize("uint8_t loop fusion 3way overwrite", SIZE, iterations, kDontShowGMeans, kDontShowPenalty );



    test_loop_overwrite4_opt( data8, SIZE, "uint8_t loop fusion 4way overwrite optimal");
    test_for_loop_blocked_overwrite4( data8, SIZE, "uint8_t for loop fusion 4way overwrite blocked");
#if !defined(_WIN32)
    test_for_loop_bzero_overwrite4( data8, SIZE, "uint8_t for loop fusion 4way overwrite bzero");
#endif
    test_for_loop_memset_overwrite4( data8, SIZE, "uint8_t for loop fusion 4way overwrite memset");
    test_for_loop_overwrite4( data8, SIZE, "uint8_t for loop fusion 4way overwrite");
    
    summarize("uint8_t loop fusion 4way overwrite", SIZE, iterations, kDontShowGMeans, kDontShowPenalty );


// int32_t
    test_loop_overwrite2_opt( data32, SIZE, "int32_t loop fusion 2way overwrite optimal");
    test_for_loop_blocked_overwrite2( data32, SIZE, "int32_t for loop fusion 2way overwrite blocked");
#if !defined(_WIN32)
    test_for_loop_bzero_overwrite2( data32, SIZE, "int32_t for loop fusion 2way overwrite bzero");
#endif
    test_for_loop_memset_overwrite2( data32, SIZE, "int32_t for loop fusion 2way overwrite memset");
    test_for_loop_overwrite2( data32, SIZE, "int32_t for loop fusion 2way overwrite");
    
    summarize("int32_t loop fusion 2way overwrite", SIZE, iterations, kDontShowGMeans, kDontShowPenalty );


    test_loop_overwrite3_opt( data32, SIZE, "int32_t loop fusion 3way overwrite optimal");
    test_for_loop_blocked_overwrite3( data32, SIZE, "int32_t for loop fusion 3way overwrite blocked");
#if !defined(_WIN32)
    test_for_loop_bzero_overwrite3( data32, SIZE, "int32_t for loop fusion 3way overwrite bzero");
#endif
    test_for_loop_memset_overwrite3( data32, SIZE, "int32_t for loop fusion 3way overwrite memset");
    test_for_loop_overwrite3( data32, SIZE, "int32_t for loop fusion 3way overwrite");
    
    summarize("int32_t loop fusion 3way overwrite", SIZE, iterations, kDontShowGMeans, kDontShowPenalty );


    test_loop_overwrite4_opt( data32, SIZE, "int32_t loop fusion 4way overwrite optimal");
    test_for_loop_blocked_overwrite4( data32, SIZE, "int32_t for loop fusion 4way overwrite blocked");
#if !defined(_WIN32)
    test_for_loop_bzero_overwrite4( data32, SIZE, "int32_t for loop fusion 4way overwrite bzero");
#endif
    test_for_loop_memset_overwrite4( data32, SIZE, "int32_t for loop fusion 4way overwrite memset");
    test_for_loop_overwrite4( data32, SIZE, "int32_t for loop fusion 4way overwrite");
    
    summarize("int32_t loop fusion 4way overwrite", SIZE, iterations, kDontShowGMeans, kDontShowPenalty );


// double
    test_loop_overwrite2_opt( dataDouble, SIZE, "double loop fusion 2way overwrite optimal");
    test_for_loop_blocked_overwrite2( dataDouble, SIZE, "double for loop fusion 2way overwrite blocked");
#if !defined(_WIN32)
    test_for_loop_bzero_overwrite2( dataDouble, SIZE, "double for loop fusion 2way overwrite bzero");
#endif
    test_for_loop_memset_overwrite2( dataDouble, SIZE, "double for loop fusion 2way overwrite memset");
    test_for_loop_overwrite2( dataDouble, SIZE, "double for loop fusion 2way overwrite");
    
    summarize("double loop fusion 2way overwrite", SIZE, iterations, kDontShowGMeans, kDontShowPenalty );

    
    test_loop_overwrite3_opt( dataDouble, SIZE, "double loop fusion 3way overwrite optimal");
    test_for_loop_blocked_overwrite3( dataDouble, SIZE, "double for loop fusion 3way overwrite blocked");
#if !defined(_WIN32)
    test_for_loop_bzero_overwrite3( dataDouble, SIZE, "double for loop fusion 3way overwrite bzero");
#endif
    test_for_loop_memset_overwrite3( dataDouble, SIZE, "double for loop fusion 3way overwrite memset");
    test_for_loop_overwrite3( dataDouble, SIZE, "double for loop fusion 3way overwrite");
    
    summarize("double loop fusion 3way overwrite", SIZE, iterations, kDontShowGMeans, kDontShowPenalty );

    
    test_loop_overwrite4_opt( dataDouble, SIZE, "double loop fusion 4way overwrite optimal");
    test_for_loop_blocked_overwrite4( dataDouble, SIZE, "double for loop fusion 4way overwrite blocked");
#if !defined(_WIN32)
    test_for_loop_bzero_overwrite4( dataDouble, SIZE, "double for loop fusion 4way overwrite bzero");
#endif
    test_for_loop_memset_overwrite4( dataDouble, SIZE, "double for loop fusion 4way overwrite memset");
    test_for_loop_overwrite4( dataDouble, SIZE, "double for loop fusion 4way overwrite");
    
    summarize("double loop fusion 4way overwrite", SIZE, iterations, kDontShowGMeans, kDontShowPenalty );



// uint8_t
    test_loop_replace_opt( data8, SIZE, "uint8_t loop fusion replace optimal");
    test_for_loop_blocked_replace2( data8, SIZE, "uint8_t for loop fusion 2way replace blocked");
    test_for_loop_blocked_replace3( data8, SIZE, "uint8_t for loop fusion 3way replace blocked");
    test_for_loop_blocked_replace4( data8, SIZE, "uint8_t for loop fusion 4way replace blocked");
#if !defined(_WIN32)
    test_for_loop_bzero_replace2( data8, SIZE, "uint8_t for loop fusion 2way replace bzero");
#endif
    test_for_loop_memset_replace2( data8, SIZE, "uint8_t for loop fusion 2way replace memset");
    test_for_loop_replace2( data8, SIZE, "uint8_t for loop fusion 2way replace");
#if !defined(_WIN32)
    test_for_loop_bzero_replace3( data8, SIZE, "uint8_t for loop fusion 3way replace bzero");
#endif
    test_for_loop_memset_replace3( data8, SIZE, "uint8_t for loop fusion 3way replace memset");
    test_for_loop_replace3( data8, SIZE, "uint8_t for loop fusion 3way replace");
#if !defined(_WIN32)
    test_for_loop_bzero_replace4( data8, SIZE, "uint8_t for loop fusion 4way replace bzero");
#endif
    test_for_loop_memset_replace4( data8, SIZE, "uint8_t for loop fusion 4way replace memset");
    test_for_loop_replace4( data8, SIZE, "uint8_t for loop fusion 4way replace");
    
    summarize("uint8_t loop fusion replace", SIZE, iterations, kDontShowGMeans, kDontShowPenalty );


// int32_t
    test_loop_replace_opt( data32, SIZE, "int32_t loop fusion replace optimal");
    test_for_loop_blocked_replace2( data32, SIZE, "int32_t for loop fusion 2way replace blocked");
    test_for_loop_blocked_replace3( data32, SIZE, "int32_t for loop fusion 3way replace blocked");
    test_for_loop_blocked_replace4( data32, SIZE, "int32_t for loop fusion 4way replace blocked");
#if !defined(_WIN32)
    test_for_loop_bzero_replace2( data32, SIZE, "int32_t for loop fusion 2way replace bzero");
#endif
    test_for_loop_memset_replace2( data32, SIZE, "int32_t for loop fusion 2way replace memset");
    test_for_loop_replace2( data32, SIZE, "int32_t for loop fusion 2way replace");
#if !defined(_WIN32)
    test_for_loop_bzero_replace3( data32, SIZE, "int32_t for loop fusion 3way replace bzero");
#endif
    test_for_loop_memset_replace3( data32, SIZE, "int32_t for loop fusion 3way replace memset");
    test_for_loop_replace3( data32, SIZE, "int32_t for loop fusion 3way replace");
#if !defined(_WIN32)
    test_for_loop_bzero_replace4( data32, SIZE, "int32_t for loop fusion 4way replace bzero");
#endif
    test_for_loop_memset_replace4( data32, SIZE, "int32_t for loop fusion 4way replace memset");
    test_for_loop_replace4( data32, SIZE, "int32_t for loop fusion 4way replace");
    
    summarize("int32_t loop fusion replace", SIZE, iterations, kDontShowGMeans, kDontShowPenalty );


// double
    test_loop_replace_opt( dataDouble, SIZE, "double loop fusion replace optimal");
    test_for_loop_blocked_replace2( dataDouble, SIZE, "double for loop fusion 2way replace blocked");
    test_for_loop_blocked_replace3( dataDouble, SIZE, "double for loop fusion 3way replace blocked");
    test_for_loop_blocked_replace4( dataDouble, SIZE, "double for loop fusion 4way replace blocked");
#if !defined(_WIN32)
    test_for_loop_bzero_replace2( dataDouble, SIZE, "double for loop fusion 2way replace bzero");
#endif
    test_for_loop_memset_replace2( dataDouble, SIZE, "double for loop fusion 2way replace memset");
    test_for_loop_overwrite2( dataDouble, SIZE, "double for loop fusion 2way replace");
#if !defined(_WIN32)
    test_for_loop_bzero_replace3( dataDouble, SIZE, "double for loop fusion 3way replace bzero");
#endif
    test_for_loop_memset_replace3( dataDouble, SIZE, "double for loop fusion 3way replace memset");
    test_for_loop_overwrite3( dataDouble, SIZE, "double for loop fusion 3way replace");
#if !defined(_WIN32)
    test_for_loop_bzero_replace4( dataDouble, SIZE, "double for loop fusion 4way replace bzero");
#endif
    test_for_loop_memset_replace4( dataDouble, SIZE, "double for loop fusion 4way replace memset");
    test_for_loop_overwrite4( dataDouble, SIZE, "double for loop fusion 4way replace");
    
    summarize("double loop fusion replace", SIZE, iterations, kDontShowGMeans, kDontShowPenalty );



// uint8_t
    test_loop_partial2_replace_opt( data8, SIZE, "uint8_t loop fusion partial2 replace optimal");
#if !defined(_WIN32)
    test_for_loop_bzero_partial2_replace( data8, SIZE, "uint8_t for loop fusion partial2 replace bzero");
#endif
    test_for_loop_memset_partial2_replace( data8, SIZE, "uint8_t for loop fusion partial2 replace memset");
    test_for_loop_partial2_replace( data8, SIZE, "uint8_t for loop fusion partial2 replace");

    summarize("uint8_t loop fusion partial2 replace", SIZE, iterations, kDontShowGMeans, kDontShowPenalty );


    test_loop_partial4_replace_opt( data8, SIZE, "uint8_t loop fusion partial4 replace optimal");
#if !defined(_WIN32)
    test_for_loop_bzero_partial4_replace( data8, SIZE, "uint8_t for loop fusion partial4 replace bzero");
#endif
    test_for_loop_memset_partial4_replace( data8, SIZE, "uint8_t for loop fusion partial4 replace memset");
    test_for_loop_partial4_replace( data8, SIZE, "uint8_t for loop fusion partial4 replace");
    test_for_loop_partial4_replaceA( data8, SIZE, "uint8_t for loop fusion partial4 replaceA");

    summarize("uint8_t loop fusion partial4 replace", SIZE, iterations, kDontShowGMeans, kDontShowPenalty );



// int32_t
    test_loop_partial2_replace_opt( data32, SIZE, "int32_t loop fusion partial2 replace optimal");
#if !defined(_WIN32)
    test_for_loop_bzero_partial2_replace( data32, SIZE, "int32_t for loop fusion partial2 replace bzero");
#endif
    test_for_loop_memset_partial2_replace( data32, SIZE, "int32_t for loop fusion partial2 replace memset");
    test_for_loop_partial2_replace( data32, SIZE, "int32_t for loop fusion partial2 replace");

    summarize("int32_t loop fusion partial2 replace", SIZE, iterations, kDontShowGMeans, kDontShowPenalty );


    test_loop_partial4_replace_opt( data32, SIZE, "int32_t loop fusion partial4 replace optimal");
#if !defined(_WIN32)
    test_for_loop_bzero_partial4_replace( data32, SIZE, "int32_t for loop fusion partial4 replace bzero");
#endif
    test_for_loop_memset_partial4_replace( data32, SIZE, "int32_t for loop fusion partial4 replace memset");
    test_for_loop_partial4_replace( data32, SIZE, "int32_t for loop fusion partial4 replace");
    test_for_loop_partial4_replaceA( data32, SIZE, "int32_t for loop fusion partial4 replaceA");

    summarize("int32_t loop fusion partial4 replace", SIZE, iterations, kDontShowGMeans, kDontShowPenalty );


// double
    test_loop_partial2_replace_opt( dataDouble, SIZE, "double loop fusion partial2 replace optimal");
#if !defined(_WIN32)
    test_for_loop_bzero_partial2_replace( dataDouble, SIZE, "double for loop fusion partial2 replace bzero");
#endif
    test_for_loop_memset_partial2_replace( dataDouble, SIZE, "double for loop fusion partial2 replace memset");
    test_for_loop_partial2_replace( dataDouble, SIZE, "double for loop fusion partial2 replace");

    summarize("double loop fusion partial2 replace", SIZE, iterations, kDontShowGMeans, kDontShowPenalty );


    test_loop_partial4_replace_opt( dataDouble, SIZE, "double loop fusion partial4 replace optimal");
#if !defined(_WIN32)
    test_for_loop_bzero_partial4_replace( dataDouble, SIZE, "double for loop fusion partial4 replace bzero");
#endif
    test_for_loop_memset_partial4_replace( dataDouble, SIZE, "double for loop fusion partial4 replace memset");
    test_for_loop_partial4_replace( dataDouble, SIZE, "double for loop fusion partial4 replace");
    test_for_loop_partial4_replaceA( dataDouble, SIZE, "double for loop fusion partial4 replaceA");

    summarize("double loop fusion partial4 replace", SIZE, iterations, kDontShowGMeans, kDontShowPenalty );





    // these run a bit faster
    iterations *= 10;

// uint8_t
    ::fill(data8, data8+smallSize, uint8_t(init_value) );
    ::fill(data8B, data8B+smallSize, uint8_t(init_value) );
    ::fill(data8C, data8C+smallSize, uint8_t(init_value) );
    
    test_loop_offset2_opt( data8, data8B, data8C, smallSize, "uint8_t loop fusion 2way offset opt");
    test_loop_offset2( data8, data8B, data8C, smallSize, "uint8_t loop fusion 2way offset");
    
    summarize("uint8_t loop fusion 2way offset", smallSize, iterations, kDontShowGMeans, kDontShowPenalty );


// int32_t
    ::fill(data32, data32+smallSize, int32_t(init_value) );
    ::fill(data32B, data32B+smallSize, int32_t(init_value) );
    ::fill(data32C, data32C+smallSize, int32_t(init_value) );
    
    test_loop_offset2_opt( data32, data32B, data32C, smallSize, "int32_t loop fusion 2way offset opt");
    test_loop_offset2( data32, data32B, data32C, smallSize, "int32_t loop fusion 2way offset");
    
    summarize("int32_t loop fusion 2way offset", smallSize, iterations, kDontShowGMeans, kDontShowPenalty );
    

// doubles
    ::fill(dataDouble, dataDouble+smallSize, double(init_value) );
    ::fill(dataDoubleB, dataDoubleB+smallSize, double(init_value) );
    ::fill(dataDoubleC, dataDoubleC+smallSize, double(init_value) );
    
    test_loop_offset2_opt( dataDouble, dataDoubleB, dataDoubleC, smallSize, "double loop fusion 2way offset opt");
    test_loop_offset2( dataDouble, dataDoubleB, dataDoubleC, smallSize, "double loop fusion 2way offset");
    
    summarize("double loop fusion 2way offset", smallSize, iterations, kDontShowGMeans, kDontShowPenalty );



    return 0;
}

// the end
/******************************************************************************/
/******************************************************************************/

