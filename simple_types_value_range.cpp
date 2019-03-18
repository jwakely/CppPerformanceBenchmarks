/*
    Copyright 2008 Adobe Systems Incorporated
    Copyright 2018 Chris Cox
    Distributed under the MIT License (see accompanying file LICENSE_1_0_0.txt
    or a copy at http://stlab.adobe.com/licenses.html )


Goal: Test compiler optimizations related to propagating the value range of simple language defined types.

Assumptions:

    1) The compiler will recognize the range of values implied by conditional tests,
        and optimize code within those conditionals according to the value ranges.
        aka: Value Range Propagation
        aka: Predicate Simplification
        can be related to value numbering and conditional constant propagation


*/

/******************************************************************************/

#include "benchmark_stdint.hpp"
#include <stddef.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <math.h>
#include <string>
#include <deque>
#include "benchmark_results.h"
#include "benchmark_timer.h"
#include "benchmark_typenames.h"

/******************************************************************************/

// this constant may need to be adjusted to give reasonable minimum times
// For best results, times should be about 1.0 seconds for the minimum test run
int iterations = 1000000;


// 8000 items, or between 8k and 64k of data
// this is intended to remain within the L2 cache of most common CPUs
#define SIZE     8000


// initial value for filling our arrays, may be changed from the command line
// BUT, the values have special meaning here, so they should not be changed
double init_value = 2.0;

/******************************************************************************/

#include "benchmark_shared_tests.h"

/******************************************************************************/

static std::deque<std::string> gLabels;

/******************************************************************************/

template <typename T>
inline void check_sum_10(T result, const std::string &label) {
    T temp = T(SIZE * 10);
    if (!tolerance_equal<T>(result,temp))
        printf("test %s failed\n", label.c_str() );
}

/******************************************************************************/

template <typename T>
void valrange_const_opt(const T *first, int count, const std::string label) {
    int i;

    start_timer();

    // This is as far as most compilers can optimize - value range
    // Nobody realizes the array is constant value, yet.
    for(i = 0; i < iterations; ++i) {
        T result = 0;
        for (int n = 0; n < count; ++n) {
            if (first[n] == T(2))
                result += 10;
            else
                result += 99;
        }
        check_sum_10<T>(result, label);
    }

    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

template <typename T>
void valrange_equal_onearg(const T *first, int count, const std::string label) {
    int i;

    start_timer();

    for(i = 0; i < iterations; ++i) {
        T result = 0;
        for (int n = 0; n < count; ++n) {
            if (first[n] == T(2))
                result += 20 / first[n];
            else
                result += 99 / first[n];
        }
        check_sum_10<T>(result, label);
    }

    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

template <typename T>
void valrange_equal_onearg2(const T *first, int count, const std::string label) {
    int i;

    start_timer();

    for(i = 0; i < iterations; ++i) {
        T result = 0;
        for (int n = 0; n < count; ++n) {
            T temp = first[n];  // explicit scalar replacement
            if (temp == T(2))
                result += 20 / temp;
            else
                result += 99 / temp;
        }
        check_sum_10<T>(result, label);
    }

    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

template <typename T>
void valrange_const_twoarg_opt(const T *first, const T *second, int count, const std::string label) {
    int i;

    start_timer();

    for(i = 0; i < iterations; ++i) {
        T result = 0;
        for (int n = 0; n < count; ++n) {
            if ((first[n] == T(2)) && (second[n] == T(3)))
                result += 10;
            else
                result += 99;
        }
        check_sum_10<T>(result, label);
    }

    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

template <typename T>
void valrange_equal_twoarg(const T *first, const T *second, int count, const std::string label) {
    int i;

    start_timer();

    for(i = 0; i < iterations; ++i) {
        T result = 0;
        for (int n = 0; n < count; ++n) {
            if (first[n] == T(2) && second[n] == T(3))
                result += 50 / (first[n] + second[n]);
            else
                result += 99 / (first[n] + second[n]);
        }
        check_sum_10<T>(result, label);
    }

    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

template <typename T>
void valrange_equal_twoarg2(const T *first, const T *second, int count, const std::string label) {
    int i;

    start_timer();

    for(i = 0; i < iterations; ++i) {
        T result = 0;
        for (int n = 0; n < count; ++n) {
            T temp1 = first[n];  // explicit scalar replacement
            T temp2 = second[n];
            if ((temp1 == T(2)) && (temp2 == T(3)))
                result += 50 / (temp1 + temp2);
            else
                result += 99 / (temp1 + temp2);
        }
        check_sum_10<T>(result, label);
    }

    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

// this version requires that the compiler re-evaluate the expression for z to determine
// that it has a constant value in one branch of the if
template <typename T>
void valrange_equal_twoarg_back(const T *first, const T *second, int count, const std::string label) {
    int i;

    start_timer();

    for(i = 0; i < iterations; ++i) {
        T result = 0;
        for (int n = 0; n < count; ++n) {
            T z = first[n] + second[n];
            if ((first[n] == T(2)) && (second[n] == T(3)))
                result += 50 / z;
            else
                result += 99 / z;
        }
        check_sum_10<T>(result, label);
    }

    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

// this version requires that the compiler re-evaluate the expression for z to determine
// that it has a constant value in one branch of the if
template <typename T>
void valrange_equal_twoarg_back2(const T *first, const T *second, int count, const std::string label) {
    int i;

    start_timer();

    for(i = 0; i < iterations; ++i) {
        T result = 0;
        for (int n = 0; n < count; ++n) {
            T temp1 = first[n];  // explicit scalar replacement
            T temp2 = second[n];
            T z = temp1 + temp2;
            if ((temp1 == T(2)) && (temp2 == T(3)))
                result += 50 / z;
            else
                result += 99 / z;
        }
        check_sum_10<T>(result, label);
    }

    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

template <typename T>
void valrange_inequal1(const T *first, int count, const std::string label) {
    int i;

    start_timer();

    for(i = 0; i < iterations; ++i) {
        T result = 0;
        for (int n = 0; n < count; ++n) {
            if (first[n] < 10) {
                // must be < 10
                if (first[n] > 20) {
                    result += 20 / first[n];    // this should be dead code, removed by the compiler
                }
                if (first[n] > 30) {
                    result += 30 / first[n];    // this should be dead code, removed by the compiler
                }
                if (first[n] > 40) {
                    result += 40 / first[n];    // this should be dead code, removed by the compiler
                }
                if (first[n] > 50) {
                    result += 50 / first[n];    // this should be dead code, removed by the compiler
                }
                if (first[n] > 60) {
                    result += 60 / first[n];    // this should be dead code, removed by the compiler
                }
                if (first[n] > 70) {
                    result += 70 / first[n];    // this should be dead code, removed by the compiler
                }
                if (first[n] > 80) {
                    result += 80 / first[n];    // this should be dead code, removed by the compiler
                }
                if (first[n] > 90) {
                    result += 90 / first[n];    // this should be dead code, removed by the compiler
                }
                if (first[n] == 100) {
                    result += 100 / first[n];    // this should be dead code, removed by the compiler
                }
                result += 10;
            }
            else {
                // must be >= 10
                // never taken
                if (first[n] < 9) {
                    result += 20 / first[n];    // this should be dead code, removed by the compiler
                }
                if (first[n] < 8) {
                    result += 30 / first[n];    // this should be dead code, removed by the compiler
                }
                if (first[n] < 7) {
                    result += 40 / first[n];    // this should be dead code, removed by the compiler
                }
                if (first[n] < 6) {
                    result += 50 / first[n];    // this should be dead code, removed by the compiler
                }
                if (first[n] < 5) {
                    result += 60 / first[n];    // this should be dead code, removed by the compiler
                }
                if (first[n] < 4) {
                    result += 70 / first[n];    // this should be dead code, removed by the compiler
                }
                if (first[n] < 3) {
                    result += 80 / first[n];    // this should be dead code, removed by the compiler
                }
                if (first[n] < 2) {
                    result += 90 / first[n];    // this should be dead code, removed by the compiler
                }
                if (first[n] == 1) {
                    result += 100 / first[n];    // this should be dead code, removed by the compiler
                }
                result += 99;
            }
        }
        check_sum_10<T>(result, label);
    }

    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

template <typename T>
void valrange_inequal2(const T *first, int count, const std::string label) {
    int i;

    start_timer();

    for(i = 0; i < iterations; ++i) {
        T result = 0;
        for (int n = 0; n < count; ++n) {
            if (first[n] >= T(50)) {
                // value must be >= 50
                // never taken
                if (first[n] <= 20) {
                    result += 20 / first[n];    // this should be dead code, removed by the compiler
                }
                if (first[n] <= 30) {
                    result += 30 / first[n];    // this should be dead code, removed by the compiler
                }
                if (first[n] < 40) {
                    result += 40 / first[n];    // this should be dead code, removed by the compiler
                }
                if (first[n] < 50) {
                    result += 50 / first[n];    // this should be dead code, removed by the compiler
                }
                if (first[n] <= 6) {
                    result += 60 / first[n];    // this should be dead code, removed by the compiler
                }
                if (first[n] <= 7) {
                    result += 70 / first[n];    // this should be dead code, removed by the compiler
                }
                if (first[n] < 8) {
                    result += 80 / first[n];    // this should be dead code, removed by the compiler
                }
                if (first[n] <= 9) {
                    result += 90 / first[n];    // this should be dead code, removed by the compiler
                }
                if (first[n] == 10) {
                    result += 100 / first[n];    // this should be dead code, removed by the compiler
                }
                result += 99;
            }
            else {
                // must be < 50
                if (first[n] > 60) {
                    result += 20 / first[n];    // this should be dead code, removed by the compiler
                }
                if (first[n] > 70) {
                    result += 30 / first[n];    // this should be dead code, removed by the compiler
                }
                if (first[n] > 80) {
                    result += 40 / first[n];    // this should be dead code, removed by the compiler
                }
                if (first[n] >= 90) {
                    result += 50 / first[n];    // this should be dead code, removed by the compiler
                }
                if (first[n] > 100) {
                    result += 60 / first[n];    // this should be dead code, removed by the compiler
                }
                if (first[n] > 110) {
                    result += 70 / first[n];    // this should be dead code, removed by the compiler
                }
                if (first[n] > 120) {
                    result += 80 / first[n];    // this should be dead code, removed by the compiler
                }
                if (first[n] == 50) {
                    result += 90 / first[n];    // this should be dead code, removed by the compiler
                }
                if (first[n] == 100) {
                    result += 100 / first[n];    // this should be dead code, removed by the compiler
                }
                result += 10;
            }
        }
        check_sum_10<T>(result, label);
    }

    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

template <typename T>
void valrange_inequal3(const T *first, int count, const std::string label) {
    int i;

    start_timer();

    for(i = 0; i < iterations; ++i) {
        T result = 0;
        for (int n = 0; n < count; ++n) {
            if (first[n] < 10) {
                // must be < 10
                // always taken
                if (first[n] < 20) {
                    if (first[n] < 30) {
                        if (first[n] < 40) {
                            if (first[n] > 50) {
                                result += 100 / first[n];    // this should be dead code, removed by the compiler
                            }
                        }
                    }
                }
                result += 10;
            }
            else {
                // must be >= 10
                // never taken
                if (first[n] > 9) {
                    if (first[n] > 8) {
                        if (first[n] > 7) {
                            if (first[n] < 5) {
                                result += 100 / first[n];    // this should be dead code, removed by the compiler
                            }
                        }
                    }
                }
                result += 99;
            }
        }
        check_sum_10<T>(result, label);
    }

    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

template <typename T>
void valrange_bool1(const T *first, int count, const std::string label) {
    int i;

    start_timer();

    for(i = 0; i < iterations; ++i) {
        T result = 0;
        for (int n = 0; n < count; ++n) {
            bool test = (first[n] < 10);    // always true
            
            if (test) {
                if (!test) {
                    result += 20 / first[n];    // this should be dead code, removed by the compiler
                }
                if (!test) {
                    result += 30 / first[n];    // this should be dead code, removed by the compiler
                }
                if (!test) {
                    result += 40 / first[n];    // this should be dead code, removed by the compiler
                }
                if (!test) {
                    result += 50 / first[n];    // this should be dead code, removed by the compiler
                }
                if (!test) {
                    result += 60 / first[n];    // this should be dead code, removed by the compiler
                }
                if (!test) {
                    result += 70 / first[n];    // this should be dead code, removed by the compiler
                }
                if (!test) {
                    result += 80 / first[n];    // this should be dead code, removed by the compiler
                }
                if (!test || !test) {
                    result += 90 / first[n];    // this should be dead code, removed by the compiler
                }
                if (!test && !test) {
                    result += 100 / first[n];    // this should be dead code, removed by the compiler
                }
                result += 10;
            }
            else {
                if (test) {
                    result += 20 / first[n];    // this should be dead code, removed by the compiler
                }
                if (test) {
                    result += 30 / first[n];    // this should be dead code, removed by the compiler
                }
                if (test) {
                    result += 40 / first[n];    // this should be dead code, removed by the compiler
                }
                if (test) {
                    result += 50 / first[n];    // this should be dead code, removed by the compiler
                }
                if (test) {
                    result += 60 / first[n];    // this should be dead code, removed by the compiler
                }
                if (test) {
                    result += 70 / first[n];    // this should be dead code, removed by the compiler
                }
                if (test) {
                    result += 80 / first[n];    // this should be dead code, removed by the compiler
                }
                if (test || test) {
                    result += 90 / first[n];    // this should be dead code, removed by the compiler
                }
                if (test && test) {
                    result += 100 / first[n];    // this should be dead code, removed by the compiler
                }
                result += 99;
            }
        }
        check_sum_10<T>(result, label);
    }

    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

template <typename T>
void valrange_bool2(const T *first, int count, const std::string label) {
    int i;

    start_timer();

    for(i = 0; i < iterations; ++i) {
        T result = 0;
        for (int n = 0; n < count; ++n) {
            bool test = (first[n] < 10);    // always true
            
            if (!test) {
                if (test) {
                    result += 20 / first[n];    // this should be dead code, removed by the compiler
                }
                if (test) {
                    result += 30 / first[n];    // this should be dead code, removed by the compiler
                }
                if (test) {
                    result += 40 / first[n];    // this should be dead code, removed by the compiler
                }
                if (test) {
                    result += 50 / first[n];    // this should be dead code, removed by the compiler
                }
                if (test) {
                    result += 60 / first[n];    // this should be dead code, removed by the compiler
                }
                if (test) {
                    result += 70 / first[n];    // this should be dead code, removed by the compiler
                }
                if (test) {
                    result += 80 / first[n];    // this should be dead code, removed by the compiler
                }
                if (test) {
                    result += 90 / first[n];    // this should be dead code, removed by the compiler
                }
                if (test) {
                    result += 100 / first[n];    // this should be dead code, removed by the compiler
                }
                result += 99;
            }
            else {
                if (!test) {
                    result += 20 / first[n];    // this should be dead code, removed by the compiler
                }
                if (!test) {
                    result += 30 / first[n];    // this should be dead code, removed by the compiler
                }
                if (!test) {
                    result += 40 / first[n];    // this should be dead code, removed by the compiler
                }
                if (!test) {
                    result += 50 / first[n];    // this should be dead code, removed by the compiler
                }
                if (!test) {
                    result += 60 / first[n];    // this should be dead code, removed by the compiler
                }
                if (!test) {
                    result += 70 / first[n];    // this should be dead code, removed by the compiler
                }
                if (!test) {
                    result += 80 / first[n];    // this should be dead code, removed by the compiler
                }
                if (!test || !test) {
                    result += 90 / first[n];    // this should be dead code, removed by the compiler
                }
                if (!test && !test) {
                    result += 100 / first[n];    // this should be dead code, removed by the compiler
                }
                result += 10;
            }
        }
        check_sum_10<T>(result, label);
    }

    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

template< typename T >
void TestOneType()
{
    gLabels.clear();
    
    std::string myTypeName( getTypeName<T>() );
    
    T dataA[SIZE];
    T dataB[SIZE];

    fill(dataA, dataA+SIZE, T(init_value));
    fill(dataB, dataB+SIZE, T(init_value+1));
    
    valrange_const_opt( dataA, SIZE, myTypeName + " value range equal1 opt");
    valrange_equal_onearg( dataA, SIZE, myTypeName + " value range equal one_arg");
    valrange_equal_onearg2( dataA, SIZE, myTypeName + " value range equal2 one_arg");
    valrange_const_twoarg_opt( dataA, dataB, SIZE, myTypeName + " value range equal two_arg opt");
    valrange_equal_twoarg( dataA, dataB, SIZE, myTypeName + " value range equal two_arg");
    valrange_equal_twoarg2( dataA, dataB, SIZE, myTypeName + " value range equal2 two_arg");
    valrange_equal_twoarg_back( dataA, dataB, SIZE, myTypeName + " value range equal two_arg back");
    valrange_equal_twoarg_back2( dataA, dataB, SIZE, myTypeName + " value range equal2 two_arg back");
    valrange_inequal1( dataA, SIZE, myTypeName + " value range inequal1");
    valrange_inequal2( dataA, SIZE, myTypeName + " value range inequal2");
    valrange_inequal3( dataA, SIZE, myTypeName + " value range inequal3");
    valrange_bool1( dataA, SIZE, myTypeName + " value range boolean1");
    valrange_bool2( dataA, SIZE, myTypeName + " value range boolean2");
    
    std::string temp1( myTypeName + " value range propagation" );
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
    if (argc > 2) init_value = (double) atof(argv[2]);

    
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
    TestOneType<long double>();

    return 0;
}

// the end
/******************************************************************************/
/******************************************************************************/
