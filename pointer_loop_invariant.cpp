/*
    Copyright 2008 Adobe Systems Incorporated
    Copyright 2018 Chris Cox
    Distributed under the MIT License (see accompanying file LICENSE_1_0_0.txt
    or a copy at http://stlab.adobe.com/licenses.html )


Goal:  Test compiler optimizations related to invariant pointers inside loops.

Assumptions:

    1) The compiler will move loop invariant calculations on pointers out of a loop.
        aka: loop invariant code motion, could be scalar replacement as well.
        
        for (i = 0; i < N; ++i)                            temp = a->b->c->d->value;
            result += a->b->c->d->value;        ==>        for (i = 0; i < N; ++i)
                                                               result += temp;
        for (i = 0; i < N; ++i)
            result += a->b[i]                   ==>        temp = a->b;
                                                           for (i = 0; i < N; ++i)
                                                              result += temp[i];
        for (i = 0; i < N; ++i)
            result += a[ b[ c[ konst ] ] ]      ==>        temp = a[ b[ c[ konst ] ] ];
                                                           for (i = 0; i < N; ++i)
                                                               result += temp;


NOTE: this is different enough from simple_type loop invariants to move it to a different file.
    Compilers used to fail this in different and amusing ways, but have improved in the past 10 years.

*/

/******************************************************************************/

#include "benchmark_stdint.hpp"
#include <stddef.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string>
#include <deque>
#include "benchmark_results.h"
#include "benchmark_timer.h"
#include "benchmark_typenames.h"

/******************************************************************************/

// this constant may need to be adjusted to give reasonable minimum times
// For best results, times should be about 1.0 seconds for the minimum test run
int iterations = 4000000;

// 8000 items, or between 8 and 64k of data
// this is intended to remain within the L2 cache of most common CPUs
#define SIZE     8000

// initial value for filling our arrays, may be changed from the command line
int32_t init_value = 7;

/******************************************************************************/

template<typename T>
struct test_struct4 {
    T value;
};

template<typename T>
struct test_struct3 {
    int16_t unused;
    test_struct4<T> *value3;
} ;

template<typename T>
struct test_struct2 {
    double unused;
    test_struct3<T> *value2;
};

template<typename T>
struct test_struct1 {
    int8_t  unused1;
    test_struct2<T> *value1;
    bool unused;
};

/******************************************************************************/

template<typename T>
struct test_struct_array1 {
    const T *array;
    bool unused2;
};

template<typename T>
struct test_struct_array2 {
    bool unused1;
    test_struct_array1<T> *struct1;
    int32_t unused2;
};

template<typename T>
struct test_struct_array3 {
    double unused1;
    test_struct_array2<T> *struct2;
    bool unused2;
};

template<typename T>
struct test_struct_array4 {
    uint8_t unused1;
    test_struct_array3<T> *struct3;
    double unused2;
};

/******************************************************************************/

template<typename T>
struct test_struct_arrayList {
    const T *array;
    size_t *list[ 5 ];
};

/******************************************************************************/

#include "benchmark_shared_tests.h"

/******************************************************************************/

template <typename T>
inline void check_sum(T result, const std::string &label) {
  T temp = (T)SIZE * (T)init_value;
  if (!tolerance_equal<T>(result,temp))
    printf("test %s failed\n", label.c_str() );
}

/******************************************************************************/

static std::deque<std::string> gLabels;

template <typename T, typename S>
void test_struct_deref_opt(const S* first, int count, const std::string label) {
    int i;

    start_timer();

    auto value = first->value;
    
    const T result = value * count;

    for(i = 0; i < iterations; ++i) {
        check_sum(result,label);
    }

    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

template <typename T, typename S>
void test_struct_deref(const S* first, int count, const std::string label) {
    int i;

    start_timer();

    for(i = 0; i < iterations; ++i) {
        T result = 0;
        for (int n = 0; n < count; ++n) {
            result += first->value;
        }
        check_sum(result,label);
    }

    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

template <typename T, typename S>
void test_struct_deref2(const S* first, int count, const std::string label) {
    int i;

    start_timer();

    for(i = 0; i < iterations; ++i) {
        T result = 0;
        for (int n = 0; n < count; ++n) {
            auto temp1 = first->value;
            result += temp1;
        }
        check_sum(result,label);
    }

    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

template <typename T, typename S>
void test_struct4_deref_opt(const S* first, int count, const std::string label) {
    int i;

    start_timer();

    auto value = first->value1->value2->value3->value;
    const T result = value * count;

    for(i = 0; i < iterations; ++i) {
        check_sum(result,label);
    }

    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

template <typename T, typename S>
void test_struct4_deref(const S* first, int count, const std::string label) {
    int i;

    start_timer();

    for(i = 0; i < iterations; ++i) {
        T result = 0;
        for (int n = 0; n < count; ++n) {
            result += first->value1->value2->value3->value;
        }
        check_sum(result,label);
    }

    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

template <typename T, typename S>
void test_struct4_deref2(const S* first, int count, const std::string label) {
    int i;

    start_timer();

    for(i = 0; i < iterations; ++i) {
        T result = 0;
        for (int n = 0; n < count; ++n) {
            auto temp1 = first->value1;
            auto temp2 = temp1->value2;
            auto temp3 = temp2->value3;
            auto temp4 = temp3->value;
            result += temp4;
        }
        check_sum(result,label);
    }

    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

template <typename T, typename S>
void test_struct_array_deref_opt(const S* first, int count, const std::string label) {
    int i;

    start_timer();

    auto array = first->array;

    for(i = 0; i < iterations; ++i) {
        T result = 0;
        for (int n = 0; n < count; ++n) {
            result += array[n];
        }
        check_sum(result,label);
    }

    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

template <typename T, typename S>
void test_struct_array_deref(const S* first, int count, const std::string label) {
    int i;

    start_timer();

    for(i = 0; i < iterations; ++i) {
        T result = 0;
        for (int n = 0; n < count; ++n) {
            result += first->array[n];
        }
        check_sum(result,label);
    }

    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

template <typename T, typename S>
void test_struct_array_deref2(const S* first, int count, const std::string label) {
    int i;

    start_timer();

    for(i = 0; i < iterations; ++i) {
        T result = 0;
        for (int n = 0; n < count; ++n) {
            auto temp = first->array;
            result += temp[n];
        }
        check_sum(result,label);
    }

    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

template <typename T, typename S>
void test_struct4_array_deref_opt(const S* first, int count, const std::string label) {
    int i;

    start_timer();

    auto array = first->struct3->struct2->struct1->array;

    for(i = 0; i < iterations; ++i) {
        T result = 0;
        for (int n = 0; n < count; ++n) {
            result += array[n];
        }
        check_sum(result,label);
    }

    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

template <typename T, typename S>
void test_struct4_array_deref(const S* first, int count, const std::string label) {
    int i;

    start_timer();

    for(i = 0; i < iterations; ++i) {
        T result = 0;
        for (int n = 0; n < count; ++n) {
            result += first->struct3->struct2->struct1->array[n];
        }
        check_sum(result,label);
    }

    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

template <typename T, typename S>
void test_struct4_array_deref2(const S* first, int count, const std::string label) {
    int i;

    start_timer();

    for(i = 0; i < iterations; ++i) {
        T result = 0;
        for (int n = 0; n < count; ++n) {
            auto temp1 = first->struct3;
            auto temp2 = temp1->struct2;
            auto temp3 = temp2->struct1;
            auto temp4 = temp3->array;
            auto temp5 = temp4[n];
            result += temp5;
        }
        check_sum(result,label);
    }

    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

template <typename T>
void test_array_deref_opt(T* first, size_t* second, size_t* third, size_t v1, int count, const std::string label) {
    int i;

    start_timer();

    auto value = first[ second[ third[ v1 ] ] ];
    const T result = value * count;

    for(i = 0; i < iterations; ++i) {
        check_sum(result,label);
    }

    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

template <typename T>
void test_array_deref(T* first, size_t* second, size_t* third, size_t v1, int count, const std::string label) {
    int i;

    start_timer();

    for(i = 0; i < iterations; ++i) {
        T result = 0;
        for (int n = 0; n < count; ++n) {
            result += first[ second[ third[ v1 ] ] ];
        }
        check_sum(result,label);
    }

    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

template <typename T>
void test_array_deref2(T* first, size_t* second, size_t* third, size_t v1, int count, const std::string label) {
    int i;

    start_timer();

    for(i = 0; i < iterations; ++i) {
        T result = 0;
        for (int n = 0; n < count; ++n) {
            auto temp1 = third[ v1 ];
            auto temp2 = second[ temp1 ];
            auto temp3 = first[ temp2 ];
            result += temp3;
        }
        check_sum(result,label);
    }

    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

template <typename T, typename S>
void test_struct_arrayList_deref_opt(const S* first, size_t v1, int count, const std::string label) {
    int i;

    start_timer();

    auto temp = first->array[ first->list[0][ first->list[1][ first->list[2][ first->list[3][ first->list[4][ v1 ] ] ] ] ] ];
    const T result = count * temp;
    
    for(i = 0; i < iterations; ++i) {
        check_sum(result,label);
    }

    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

template <typename T, typename S>
void test_struct_arrayList_deref(const S* first, size_t v1, int count, const std::string label) {
    int i;

    start_timer();

    for(i = 0; i < iterations; ++i) {
        T result = 0;
        for (int n = 0; n < count; ++n) {
            result += first->array[ first->list[0][ first->list[1][ first->list[2][ first->list[3][ first->list[4][ v1 ] ] ] ] ] ];
        }
        check_sum(result,label);
    }

    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

template <typename T, typename S>
void test_struct_arrayList_deref2(const S* first, size_t v1, int count, const std::string label) {
    int i;

    start_timer();

    for(i = 0; i < iterations; ++i) {
        T result = 0;
        for (int n = 0; n < count; ++n) {
            auto temp1 = first->list[4][ v1 ];
            auto temp2 = first->list[3][ temp1 ];
            auto temp3 = first->list[2][ temp2 ];
            auto temp4 = first->list[1][ temp3 ];
            auto temp5 = first->list[0][ temp4 ];
            auto temp6 = first->array[ temp5 ];
            result += temp6;
        }
        check_sum(result,label);
    }

    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

template <typename T, typename S>
void test_struct_arrayList_deref3(const S* first, size_t v1, int count, const std::string label) {
    int i;

    start_timer();

    for(i = 0; i < iterations; ++i) {
        T result = 0;
        for (int n = 0; n < count; ++n) {
            auto tempA = first->list[4];
            auto tempB = first->list[3];
            auto tempC = first->list[2];
            auto tempD = first->list[1];
            auto tempE = first->list[0];
            auto tempF = first->array;
            
            auto temp1 = tempA[ v1 ];
            auto temp2 = tempB[ temp1 ];
            auto temp3 = tempC[ temp2 ];
            auto temp4 = tempD[ temp3 ];
            auto temp5 = tempE[ temp4 ];
            auto temp6 = tempF[ temp5 ];
            
            result += temp6;
        }
        check_sum(result,label);
    }

    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/
/******************************************************************************/

const size_t kArrayTestSize = 30;

template< typename T >
void TestOneType(T temp)
{
    std::string myTypeName( getTypeName<T>() );
    
    gLabels.clear();
    
    T data[SIZE];

    ::fill(data, data+SIZE, T(init_value));
    
    
    T var1_1, var1_2, var1_3, var1_4;
    var1_1 = T(temp);
    var1_2 = var1_1 * T(2);
    var1_3 = var1_1 + T(2);
    var1_4 = var1_1 + var1_2 / var1_3;
    
    test_struct1<T> test1;
    test_struct2<T> test2;
        test1.value1 = &test2;
    test_struct3<T> test3;
        test2.value2 = &test3;
    test_struct4<T> test4;
        test3.value3 = &test4;
        test4.value = T(init_value);
    
    
    test_struct_array1<T>    testSA1;
        testSA1.array = data;
    test_struct_array2<T>    testSA2;
        testSA2.struct1 = &testSA1;
    test_struct_array3<T>    testSA3;
        testSA3.struct2 = &testSA2;
    test_struct_array4<T>    testSA4;
        testSA4.struct3 = &testSA3;
    
    
    size_t index1( size_t(temp) % kArrayTestSize );
    
    T test_first[ kArrayTestSize ];
    size_t test_second[ kArrayTestSize ];
    size_t test_third[ kArrayTestSize ];
    
    ::fill(test_first, test_first+kArrayTestSize, T(init_value));
    ::fill(test_second, test_second+kArrayTestSize, size_t(size_t(var1_3) % kArrayTestSize));
    ::fill(test_third, test_third+kArrayTestSize, size_t( size_t(var1_4) % kArrayTestSize));
    
    test_struct_arrayList<T> test_arrayList;
    test_arrayList.array = test_first;
    test_arrayList.list[0] = test_second;
    test_arrayList.list[1] = test_third;
    test_arrayList.list[2] = test_second;
    test_arrayList.list[3] = test_third;
    test_arrayList.list[4] = test_second;
    
    test_struct_array_deref_opt< T, test_struct_array1<T> > (&testSA1, SIZE, myTypeName + " struct array deref optimal");
    test_struct_array_deref< T, test_struct_array1<T> > (&testSA1, SIZE, myTypeName + " struct array deref");
    test_struct_array_deref2< T, test_struct_array1<T> > (&testSA1, SIZE, myTypeName + " struct array deref2");
    
    test_struct4_array_deref_opt< T, test_struct_array4<T> > (&testSA4, SIZE, myTypeName + " struct4 array deref optimal");
    test_struct4_array_deref< T, test_struct_array4<T> > (&testSA4, SIZE, myTypeName + " struct4 array deref");
    test_struct4_array_deref2< T, test_struct_array4<T> > (&testSA4, SIZE, myTypeName + " struct4 array deref2");

    test_struct_deref_opt< T, test_struct4<T> > (&test4, SIZE, myTypeName + " struct deref optimal");
    test_struct_deref< T, test_struct4<T> > (&test4, SIZE, myTypeName + " struct deref");
    test_struct_deref2< T, test_struct4<T> > (&test4, SIZE, myTypeName + " struct deref2");

    test_struct4_deref_opt< T, test_struct1<T> > (&test1, SIZE, myTypeName + " struct4 deref optimal");
    test_struct4_deref< T, test_struct1<T> > (&test1, SIZE, myTypeName + " struct4 deref");
    test_struct4_deref2< T, test_struct1<T> > (&test1, SIZE, myTypeName + " struct4 deref2");
    
    test_array_deref_opt<T>( test_first, test_second, test_third, index1, SIZE, myTypeName + " array deref optimal");
    test_array_deref<T>( test_first, test_second, test_third, index1, SIZE, myTypeName + " array deref");
    test_array_deref2<T>( test_first, test_second, test_third, index1, SIZE, myTypeName + " array deref2");

    test_struct_arrayList_deref_opt< T, test_struct_arrayList<T> > (&test_arrayList, index1, SIZE, myTypeName + " struct array list deref optimal");
    test_struct_arrayList_deref< T, test_struct_arrayList<T> > (&test_arrayList, index1, SIZE, myTypeName + " struct array list deref");
    test_struct_arrayList_deref2< T, test_struct_arrayList<T> > (&test_arrayList, index1, SIZE, myTypeName + " struct array list deref2");
    test_struct_arrayList_deref3< T, test_struct_arrayList<T> > (&test_arrayList, index1, SIZE, myTypeName + " struct array list deref3");

    std::string temp1( myTypeName + " pointer loop invariant");
    summarize(temp1.c_str(), SIZE, iterations, kDontShowGMeans, kDontShowPenalty );
}

/******************************************************************************/

int main(int argc, char** argv) {
    int32_t temp = 5;
    
    // output command for documentation:
    int i;
    for (i = 0; i < argc; ++i)
        printf("%s ", argv[i] );
    printf("\n");

    if (argc > 1) iterations = atoi(argv[1]);
    if (argc > 2) init_value = (int32_t) atoi(argv[2]);
    if (argc > 3) temp = (int32_t)atoi(argv[3]);


    TestOneType<uint8_t>(temp);
    TestOneType<int8_t>(temp);
    TestOneType<uint16_t>(temp);
    TestOneType<int16_t>(temp);
    TestOneType<uint32_t>(temp);
    TestOneType<int32_t>(temp);

    iterations /= 8;
    TestOneType<uint64_t>(temp);
    TestOneType<int64_t>(temp);
    TestOneType<float>(temp);
    TestOneType<double>(temp);
    
    
    return 0;
}

// the end
/******************************************************************************/
/******************************************************************************/
