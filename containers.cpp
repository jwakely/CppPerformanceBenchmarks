/*
    Copyright 2007-2008 Adobe Systems Incorporated
    Copyright 2018-2022 Chris Cox
    Distributed under the MIT License (see accompanying file LICENSE_1_0_0.txt
    or a copy at http://stlab.adobe.com/licenses.html)


Goal: examine the performance of operations with various standard containers


Assumptions:
    
    std::vector and std::array should perform almost identically to a pointer or array
    
    reverse iteration should be similar in performance to forward iteration, for the containers that support it
    
    std::deque should perform just a little slower than a vector/array/pointer
        std::deque needs more bounds checking, and occasional dereferences
        std::deque should not be hugely slower than a vector/array/pointer
    
    std::forward_list should perform similarly to a naive singly linked list
        slower than vector/array/pointer
        std::forward_list requires a dereference for each step (high chance of a cache miss)
    
    std::list should perform similarly to a naive doubly linked list
        in most cases std::list will be slower than std::forward_list
        std::list requires a dereference for each step, and more data per node (higher chance of a cache miss)
    
    iterating a std::set,std::map, or std::multi* is slow compared to iterating lists, deques or vectors
        std::set and std::map require navigating a tree structure - several dereferences possible for each step (very high chance of a cache miss)
    
    iterating an unordered/hashmap can be similar in performance to iterating a linked list
        this can be improved with pooled allocation and unordered iterators
    
    duplicating an pointer/std::array/std:vector/std::deque should all have similar performance
    
    duplicating linked lists will be slower than a vector/array/pointer, because they need many allocations
        but pooled allocation can help a lot
    
    duplicating sets, maps, and hashmaps will be slower than vectors/arrays/pointers, because they need many allocations
        but pooled allocation can help a lot
    
    for a large number of items, inserting items into std::set and std::map are very slow compared to lists, deques or vectors
        more allocations, more dereferencing, more tree balancing
    
    std::vector and std::deque push_back should perform better than a std::list or other linked lists
        std::vector has much fewer allocations, even though they are large and need occasional copies
        std::deque has fewer allocations, no copies (can outperform vector)
        linked lists have to make an allocation per item added -- pooled allocation can help
    
    std::deque push_front should perform better than a std::list or other linked lists
        std::deque allocates blocks of items, so has fewer allocations to make

    std::forward_list push_front should perform equal to or better than a naive singly linked list

    std::list push_back and push_front should perform equal to or better than a naive doubly linked list
    
    ordered insertion into a std::map/set/multimap/multiset will generally be faster than random order insertion
        better reuse of cached data, plus slightly better construction of red-black trees
    
    insertion into a std::unordered_map/unordered_set/unordered_multi/hashmap should be about the same speed regardless of order
    
    deleting a pointer/array/vector or deque should be very fast
    
    deleting linked lists, sets, and maps will be slower because they are deleting many small allocations
        but pooled allocation can help a lot
    
    erasing or clearing all entries on a vector/array or deque should be very fast
    
    erasing or clearing all entries on a linked list will be slower, partly due to iteration, and partly due to deleting many small allocations
    
    erasing or clearing all entries of a set/map/multi/unordered/hashmap can be slowest due to extra structure and many small allocations deleted
        (but pooled allocation can help a lot)

    std::deque pop_front should be much faster than for linked lists
        (but pooled allocation for the lists can help a lot)
    
    std::vector and std:deque pop_back should have comparable performance, and both be faster than linked lists
        but pooled allocation for the lists can help a lot
    
    sorted vectors and sorted deques are still relatively slow for finding items compared to sets/maps/multi/unordered/hashmaps
        even using binary search instead of linear search
        regardless of the search order
    
    for a large number of items, unordered/hashmaps are generally going to be fastest for finding items
    
    a sorted std::vector or std::deque can be fast when erasing items in sorted order
        though vector will only perform well in reverse order (aka pop_back), and horribly in forward order (aka pop_front)
    
    a sorted std::vector or std::deque will be very slow when erasing items in random order
    
    for a large number of items, unordered/hashmaps are generally going to be fastest for finding and erasing specific items
        and pooled allocation can help a lot
    
    
        



Notes:

    Something is very, very wrong with MSVC's implementation of deque
        it is incredibly slow, and sometimes behaves closer to a linked list than a deque
    
    Many other data structures are also much slower under MSVC than gcc or clang/llvm
        Do they have a suballocator bug, bad STL code, or just poor code generation?
    
    iterating a vector/array/pointer is literally just incrementing a pointer
        vector/array/pointer should be fastest among containers for iteration

    iterating a deque involves some bounds checks, making it slightly slower than vector/array/pointer
        deque operations will sometimes be calculation bound

    iterating a linked list involves dereferences at every step
        speed depends on the size and memory layout of the items
        linked list will be memory/cache bound (lots of cache misses!)
    
    iterating a map/set/multi involves multiple dereferences at every step
        speed depends on the size and memory layout of the items in the containers
        map/set/multi will be memory/cache bound (lots of cache misses!)

    iterating unordered/hashmaps involves dereferences
        speed depends on the size and memory layout of the items
        cache misses can be somewhat better or worse than linked lists, depending on implementation

    reverse iteration should be similar in performance to forward iteration, for the containers that support reverse iterators
        on some CPUs reverse iteration may be slower due to poor cache prediction
    
    std::vector push_back involves occasional copies and a few reallocations of large blocks
        time is spent in copies and allocation

    std::deque push_back and push_front involve no copies and allocation of relatively few, smaller blocks
        time is spent in allocation and iteration
        smaller allocations allow for optimizations using OS or library suballocator
            this gives it some advantage over vector (except under MSVC)
     
    linked list push_back and push_front involve a lot of small allocations
        majority of the time is spent in the allocator

    map/set/multi insertion involves tree traversal, tree balancing, plus many small allocations (items plus tree structure)

    unordered/hashmap insertion involves hashing, a few reallocations, a few rehashes, and small allocations
        pooled allocation can help quite a bit
    
    deletion of a pointer/vector/deque involves deleting one, or very few, large allocations
        with an exception for deque under MSVC
    
    deletion of a linked list, set, map, unordered, or hashmap will involve deleting many small allocations
        this takes a lot of time if the allocations were not pooled
    
    erasing or clearing all entries of a vector/deque is very fast because it is deleting one, or very few, large allocations
        with an exception for deque under MSVC
    
    erasing or clearing all entries of a linked list, unordered, or hashmap will be slow
        involves dereferencing of pointers (cache misses), plus deleting many small allocations
        goes much faster if the allocations were pooled
    
    erasing or clearing all entries of a set, map, or multi will be slow
        involves dereferencing of pointers (cache misses), deleting many small allocations, plus deleting and rebalancing tree structures
        goes much faster if the allocations were pooled
    
    pop_front on a deque just involves moving a pointer, and sometimes deleting a large allocation
    
    pop_front on a linked list involves some pointer chasing (cache misses), and a deletion for every object
        but pooled allocation can help a lot
    
    pop_back on a vector just involves moving a pointer
    
    pop_back on a deque should be the same as pop_front: move a pointer, sometimes delete a large block
    
    pop_back on a linked list involves some pointer chasing (cache misses), and a deletion for every object
        but pooled allocation can help a lot
    
    finding an item in an unsorted vector/array/deque means a linear search - which will be slow
    
    finding an item in a sorted vector/array/deque can use binary search (std::lower_bound) and be a bit faster
    
    finding an item in a linked list means a linear search, plus pointer chasing (cache misses) - and will be very slow
    
    finding an item in a set/map/multi means traversing a tree, pointer chasing (cache misses), and multiple comparisons
    
    finding an item in an unordered/hashmap involves hashing, a few comparisons, and a small amount of pointer chasing (cache misses)
        but should be fastest among common containers
    
    erasing an item requires finding that entry, then deleting it
    
    erasing an item in an unsorted vector/array/deque means a linear search, and copying data (to fill the hole) - which will be slow
    
    erasing an item in a sorted vector/array/deque can use binary search (std::lower_bound) and be a bit faster
    
    erasing an item in a linked list means a linear search, pointer chasing (cache misses), and deleting small allocations
        but pooled allocation can help
    
    erasing an item in a set/map/multi means traversing a tree, pointer chasing (cache misses), multiple comparisons,
        tree rebalancing, and deleting small allocations
    
    erasing an item in an unordered/hashmap involves hashing, a few comparisons,
        a small amount of pointer chasing (cache misses), and deleting a small allocation
        but pooled allocation can help a lot

    Accumulate is usually a little slower than copy
        copy has stalls on load and store
            but these are likely to execute out of order on current processors, and maximize cache coherency in vector/array/deque
        accumulate tends to serialize adds, hitting stalls on the floating point adds
            these won't execute out of order nearly as well as loads/stores
                which tend to be optimized on most common processors (because it's a common occurrence)
            FP tends to have fewer reservations, fewer units compared to integer
            accumulate appears serialized
        vectorization of the accumulate may help considerably on contiguous containers
        splitting the accumulation variable may help, depending on the processor and compiler

    hash map lookup performance is best with a load factor between 0.5 and 1.5
        Larger load factors improve the insertion time (because we have to rehash the table less often),
            but increase the search time because more items are linked per hash cell
        








TODO - mixed operations tests
    
    Stroustrup's Bentley test
    Alex Stepanov also requested this
    https://www.stroustrup.com/bs_faq.html
    https://www.stroustrup.com/Software-for-infrastructure.pdf
        test for small and large data sizes (custom structure, strings, etc. make sure operator== takes time on large structs)
            insert random values into sorted container
            remove random values from sorted container
    
    Database mix for associative containers (key/value pairs)
        test for both small and large data sizes (custom structure, strings, etc. make sure operator== takes time on large structs)
            insert random order unique values into container
            lookup random order unique values from container
                including some keys that do not exist!
            iterate container - accumulate (simulating search or garbage collection)
            remove random order unique values from container
            insert, lookup, remove random order mix   -- TODO
            duplicate/copy container
            clear container
            delete container



TODO - large data structure for value ???
        May require significant change in the test routines

TODO - large data structure for keys ???
        May require significant change in the test routines
        
*/

/******************************************************************************/

#include "benchmark_stdint.hpp"
#include <stddef.h>
#include <stdio.h>
#include <time.h>
#include <math.h>
#include <stdlib.h>
#include <string>
#include <algorithm>
#include <cassert>

#include <array>
#include <vector>
#include <deque>
#include <forward_list>
#include <list>
#include <set>
#include <map>
#include <unordered_set>
#include <unordered_map>

#include "benchmark_containers.h"
#include "benchmark_algorithms.h"
#include "benchmark_results.h"
#include "benchmark_timer.h"
#include "benchmark_typenames.h"

/******************************************************************************/

// Should we test the simple container find and erase times?
// THIS IS VERY SLOW - it adds over 12 hours to the run time!
#ifndef TEST_SLOW_FINDS
#define TEST_SLOW_FINDS        0
#endif


// this constant may need to be adjusted to give reasonable minimum times
// For best results, times should be about 1.0 seconds for the minimum test run
int base_iterations = 500000;
int iterations = base_iterations;


// 8000 items, or about 64k of data
#define SIZE     8000


/******************************************************************************/

std::deque<std::string> gLabels;

/******************************************************************************/
/******************************************************************************/

// our accumulator function template, using iterators or pointers
template <class Iterator, class Number>
Number myaccumulate(Iterator first, Iterator last, Number result) {
    while (first != last) result = result + *first++;
    return result;
}

/******************************************************************************/

// our accumulator function template, using iterators or pointers
// only needed for raw pointer/array
template <class Iterator, class Number>
Number myaccumulate_reverse(Iterator first, Iterator last, Number result) {
    while (last != first) result = result + *(--last);
    return result;
}

/******************************************************************************/

// our accumulator function template, using iterators that return pairs
template <class Iterator, class Number>
Number myaccumulate_pair(Iterator first, Iterator last, Number result) {
    while (first != last) result = result + (first++)->second;
    return result;
}

/******************************************************************************/
/******************************************************************************/

template< typename value_T >
void test_copy_array(const value_T* master_begin, const value_T* master_end, const std::string &label) {
    int i;
    
    ptrdiff_t length = master_end - master_begin;
    value_T *myArray = new value_T[ length ];
    
    double sum = 0.0;
    sum = myaccumulate( master_begin, master_end, sum );
    
    start_timer();
    
    for (i = 0; i < iterations; ++i)
        {
        auto master_ptr = master_begin;
        value_T *copy_begin = myArray;
        
        while (master_ptr != master_end)
            *copy_begin++ = *master_ptr++;
        }
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
    
    double test = 0.0;
    test = myaccumulate( myArray, myArray+length, test );
    
    if (test != sum)
        printf("test %i failed\n", current_test);
    
    delete[] myArray;
}

/******************************************************************************/

template< typename value_T >
void test_copy_stdarray(const value_T* master_begin, const value_T* master_end, const std::string &label) {
    
    std::array<double,SIZE> myVector;
    
    double sum = 0.0;
    sum = myaccumulate( master_begin, master_end, sum );
    
    start_timer();
    
    for (int i = 0; i < iterations; ++i)
        {
        auto master_ptr = master_begin;
        auto copy_begin = myVector.begin();
        
        while (master_ptr != master_end)
            *copy_begin++ = *master_ptr++;
        }
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
    
    double test = 0.0;
    test = myaccumulate( myVector.cbegin(), myVector.cend(), test );
    
    if (test != sum)
        printf("test %i failed\n", current_test);
}

/******************************************************************************/

template<typename value_T, class testContainerType>
void test_copy(const value_T* master_begin, const value_T* master_end, const std::string &label) {
    
    testContainerType *myVector = new testContainerType;
    ptrdiff_t length = master_end - master_begin;
    myVector->resize( length );
    
    double sum = 0.0;
    sum = myaccumulate( master_begin, master_end, sum );
    
    start_timer();
    
    for (int i = 0; i < iterations; ++i)
        {
        auto master_ptr = master_begin;
        auto copy_begin = myVector->begin();
        
        while (master_ptr != master_end)
            *copy_begin++ = *master_ptr++;
        }
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
    
    double test = 0.0;
    test = myaccumulate( myVector->cbegin(), myVector->cend(), test );
    
    if (test != sum)
        printf("test %i failed\n", current_test);
        
    delete myVector;

}

/******************************************************************************/
/******************************************************************************/

template<typename value_T>
void test_accum_array(const value_T* master_begin, const value_T* master_end, const std::string &label) {
    int i;
    
    ptrdiff_t length = master_end - master_begin;
    value_T *myArray = new value_T[ length ];
    
    auto master_ptr = master_begin;
    value_T *copy_begin = myArray;
    double masterSum = 0.0;
    
    // insert the items and sum for comparison
    while (master_ptr != master_end) {
        *copy_begin++ = *master_ptr;
        masterSum += *master_ptr;
        master_ptr++;
        }
    
    start_timer();
    
    for (i = 0; i < iterations; ++i)
        {
        double testSum = 0.0;
        const value_T *first = myArray;
        const value_T *last = myArray+length;
        
        testSum = myaccumulate(first,last,testSum);
        
        if (testSum != masterSum)
            printf("test %i failed\n", current_test);
        }
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
    
    delete[] myArray;

}

/******************************************************************************/

template <typename value_T>
void test_accum_stdarray(const value_T* master_begin, const value_T* master_end, const std::string &label) {
    
    std::array<value_T,SIZE> myVector;
    auto master_ptr = master_begin;
    auto copy_begin = myVector.begin();
    double masterSum = 0.0;
    
    // copy values and sum for comparison
    while (master_ptr != master_end) {
        *copy_begin++ = *master_ptr;
        masterSum += *master_ptr;
        master_ptr++;
        }
    
    start_timer();
    
    for (int i = 0; i < iterations; ++i)
        {
        double testSum = 0.0;
        auto first = myVector.cbegin();
        auto last = myVector.cend();
        
        testSum = myaccumulate(first,last,testSum);
        
        if (testSum != masterSum)
            printf("test %i failed\n", current_test);
        }
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

template< typename value_T>
void test_accum_array_reverse(const value_T* master_begin, const value_T* master_end, const std::string &label) {
    int i;
    
    ptrdiff_t length = master_end - master_begin;
    value_T *myArray = new value_T[ length ];
    
    auto master_ptr = master_begin;
    value_T *copy_begin = myArray;
    double masterSum = 0.0;
    
    // insert the items and sum for comparison
    while (master_ptr != master_end) {
        *copy_begin++ = *master_ptr;
        masterSum += *master_ptr;
        master_ptr++;
        }
    
    start_timer();
    
    for (i = 0; i < iterations; ++i)
        {
        double testSum = 0.0;
        const value_T *first = myArray;
        const value_T *last = myArray+length;
        
        testSum = myaccumulate_reverse(first,last,testSum);
        
        if (testSum != masterSum)
            printf("test %i failed\n", current_test);
        }
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
    
    delete[] myArray;

}

/******************************************************************************/

template< typename value_T>
void test_accum_stdarray_reverse(const value_T* master_begin, const value_T* master_end, const std::string &label) {
    
    std::array<value_T,SIZE> myVector;
    auto master_ptr = master_begin;
    auto copy_begin = myVector.begin();
    double masterSum = 0.0;
    
    // copy values and sum for comparison
    while (master_ptr != master_end) {
        *copy_begin++ = *master_ptr;
        masterSum += *master_ptr;
        master_ptr++;
        }
    
    start_timer();
    
    for (int i = 0; i < iterations; ++i)
        {
        double testSum = 0.0;
        auto first = myVector.crbegin();
        auto last = myVector.crend();
        
        testSum = myaccumulate(first,last,testSum);
        
        if (testSum != masterSum)
            printf("test %i failed\n", current_test);
        }
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
}

/******************************************************************************/

template< typename value_T, class testContainerType >
void test_accum(const value_T* master_begin, const value_T* master_end, const std::string &label) {
    int i;
    
    testContainerType *myVector = new testContainerType;
    ptrdiff_t length = master_end - master_begin;
    myVector->resize( length );
    
    auto master_ptr = master_begin;
    auto copy_begin = myVector->begin();
    double masterSum = 0.0;
    
    // insert the items and sum for comparison
    while (master_ptr != master_end) {
        *copy_begin++ = *master_ptr;
        masterSum += *master_ptr;
        master_ptr++;
        }
    
    start_timer();
    
    for (i = 0; i < iterations; ++i)
        {
        double testSum = 0.0;
        auto first = myVector->cbegin();
        auto last = myVector->cend();
        
        testSum = myaccumulate(first,last,testSum);
        
        if (testSum != masterSum)
            printf("test %i failed\n", current_test);
        }
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
        
    delete myVector;

}

/******************************************************************************/

template< typename value_T, class testContainerType >
void test_accum_unordered(const value_T* master_begin, const value_T* master_end, const std::string &label) {
    int i;
    
    testContainerType *myVector = new testContainerType;
    ptrdiff_t length = master_end - master_begin;
    myVector->resize( length );
    
    auto master_ptr = master_begin;
    auto copy_begin = myVector->begin();
    double masterSum = 0.0;
    
    // insert the items and sum for comparison
    while (master_ptr != master_end) {
        *copy_begin++ = *master_ptr;
        masterSum += *master_ptr;
        master_ptr++;
        }
    
    start_timer();
    
    for (i = 0; i < iterations; ++i)
        {
        double testSum = 0.0;
        auto first = myVector->cubegin();
        auto last = myVector->cuend();
        
        testSum = myaccumulate(first,last,testSum);
        
        if (testSum != masterSum)
            printf("test %i failed\n", current_test);
        }
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
        
    delete myVector;

}

/******************************************************************************/

template< typename value_T, class testContainerType >
void test_accum_reverse(const value_T* master_begin, const value_T* master_end, const std::string &label) {
    int i;
    
    testContainerType *myVector = new testContainerType;
    ptrdiff_t length = master_end - master_begin;
    myVector->resize( length );
    
    auto master_ptr = master_begin;
    auto copy_begin = myVector->begin();
    double masterSum = 0.0;
    
    // insert the items and sum for comparison
    while (master_ptr != master_end) {
        *copy_begin++ = *master_ptr;
        masterSum += *master_ptr;
        master_ptr++;
        }
    
    start_timer();
    
    for (i = 0; i < iterations; ++i)
        {
        double testSum = 0.0;
        auto first = myVector->crbegin();
        auto last = myVector->crend();
        
        testSum = myaccumulate(first,last,testSum);
        
        if (testSum != masterSum)
            printf("test %i failed\n", current_test);
        }
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );
        
    delete myVector;

}

/******************************************************************************/

template< typename value_T, class testContainerType >
void test_accum_set(const value_T* master_begin, const value_T* master_end, const std::string &label) {
    int i;
    
    // this container only allow one copy of a value, and we'll have aliasing
    if ( sizeof(value_T) < 2)
        return;

    testContainerType testSet;
    
    auto master_ptr = master_begin;
    double masterSum = 0.0;
    
    // insert the items and sum for comparison
    while (master_ptr != master_end) {
        testSet.insert( *master_ptr );
        masterSum += *master_ptr;
        master_ptr++;
        }
    
    
    start_timer();
    
    for (i = 0; i < iterations; ++i) {
        double testSum = 0.0;
        auto first = testSet.cbegin();
        auto last = testSet.cend();
        
        testSum = myaccumulate(first,last,testSum);
        
        if (testSum != masterSum)
            printf("test %i failed\n", current_test);
    }
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );

}

/******************************************************************************/

template< typename value_T, class testContainerType >
void test_accum_set_reverse(const value_T* master_begin, const value_T* master_end, const std::string &label) {
    int i;
    
    // this container only allow one copy of a value, and we'll have aliasing
    if ( sizeof(value_T) < 2)
        return;

    testContainerType testSet;
    
    auto master_ptr = master_begin;
    double masterSum = 0.0;
    
    // insert the items and sum for comparison
    while (master_ptr != master_end) {
        testSet.insert( *master_ptr );
        masterSum += *master_ptr;
        master_ptr++;
        }
    
    
    start_timer();
    
    for (i = 0; i < iterations; ++i) {
        double testSum = 0.0;
        auto first = testSet.crbegin();
        auto last = testSet.crend();
        
        testSum = myaccumulate(first,last,testSum);
        
        if (testSum != masterSum)
            printf("test %i failed\n", current_test);
    }
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );

}

/******************************************************************************/

template< typename value_T, class testContainerType >
void test_accum_multimap(const value_T* master_begin, const value_T* master_end, const std::string &label) {
    int i;
    
    // this container only allow one copy of a value, and we'll have aliasing
    if ( sizeof(value_T) < 2)
        return;

    testContainerType testSet;
    
    auto master_ptr = master_begin;
    double masterSum = 0.0;
    
    // insert the items and sum for comparison
    while (master_ptr != master_end) {
        testSet.insert( std::pair<value_T,value_T>(*master_ptr,*master_ptr) );
        masterSum += *master_ptr;
        master_ptr++;
        }
    
    
    start_timer();
    
    for (i = 0; i < iterations; ++i) {
        double testSum = 0.0;
        auto first = testSet.cbegin();
        auto last = testSet.cend();
        
        testSum = myaccumulate_pair(first,last,testSum);
        
        if (testSum != masterSum)
            printf("test %i failed\n", current_test);
    }
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );

}

/******************************************************************************/

template< typename value_T, class testContainerType >
void test_accum_multimap_reverse(const value_T* master_begin, const value_T* master_end, const std::string &label) {
    int i;
    
    // this container only allow one copy of a value, and we'll have aliasing
    if ( sizeof(value_T) < 2)
        return;

    testContainerType testSet;
    
    auto master_ptr = master_begin;
    double masterSum = 0.0;
    
    // insert the items and sum for comparison
    while (master_ptr != master_end) {
        testSet.insert( std::pair<value_T,value_T>(*master_ptr,*master_ptr) );
        masterSum += *master_ptr;
        master_ptr++;
        }
    
    
    start_timer();
    
    for (i = 0; i < iterations; ++i) {
        double testSum = 0.0;
        auto first = testSet.crbegin();
        auto last = testSet.crend();
        
        testSum = myaccumulate_pair(first,last,testSum);
        
        if (testSum != masterSum)
            printf("test %i failed\n", current_test);
    }
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );

}

/******************************************************************************/

template< typename value_T, class testContainerType >
void test_accum_map(const value_T* master_begin, const value_T* master_end, const std::string &label) {
    int i;
    
    // this container only allow one copy of a value, and we'll have aliasing
    if ( sizeof(value_T) < 2)
        return;

    testContainerType testMap;
    
    auto master_ptr = master_begin;
    double masterSum = 0.0;
    
    // insert the items and sum for comparison
    while (master_ptr != master_end) {
        testMap[ *master_ptr ] = *master_ptr;
        masterSum += *master_ptr;
        master_ptr++;
        }
    
    
    start_timer();
    
    for (i = 0; i < iterations; ++i) {
        double testSum = 0.0;
        auto first = testMap.cbegin();
        auto last = testMap.cend();
        
        testSum = myaccumulate_pair(first,last,testSum);
        
        if (testSum != masterSum)
            printf("test %i failed\n", current_test);
    }
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );

}

/******************************************************************************/

template< typename value_T, class testContainerType >
void test_accum_map_reverse(const value_T* master_begin, const value_T* master_end, const std::string &label) {
    int i;
    
    // this container only allow one copy of a value, and we'll have aliasing
    if ( sizeof(value_T) < 2)
        return;

    testContainerType testMap;
    
    auto master_ptr = master_begin;
    double masterSum = 0.0;
    
    // insert the items and sum for comparison
    while (master_ptr != master_end) {
        testMap[ *master_ptr ] = *master_ptr;
        masterSum += *master_ptr;
        master_ptr++;
        }
    
    
    start_timer();
    
    for (i = 0; i < iterations; ++i) {
        double testSum = 0.0;
        auto first = testMap.crbegin();
        auto last = testMap.crend();
        
        testSum = myaccumulate_pair(first,last,testSum);
        
        if (testSum != masterSum)
            printf("test %i failed\n", current_test);
    }
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );

}

/******************************************************************************/

template< typename value_T, class testContainerType >
void test_accum_simplehashmap(const value_T* master_begin, const value_T* master_end, const std::string &label) {
    int i;
    
    // this container only allow one copy of a value, and we'll have aliasing
    if ( sizeof(value_T) < 2)
        return;

    testContainerType testMap;
    
    auto master_ptr = master_begin;
    double masterSum = 0.0;
    
    // insert the items and sum for comparison
    while (master_ptr != master_end) {
        testMap[ *master_ptr ] = *master_ptr;
        masterSum += *master_ptr;
        master_ptr++;
        }
    
    
    start_timer();
    
    for (i = 0; i < iterations; ++i) {
        double testSum = 0.0;
        auto first = testMap.cbegin();
        auto last = testMap.cend();
        
        testSum = myaccumulate_pair(first,last,testSum);
        
        if (testSum != masterSum)
            printf("test %i failed\n", current_test);
    }
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );

}

/******************************************************************************/

template< typename value_T, class testContainerType >
void test_accum_simplehashmap_unordered(const value_T* master_begin, const value_T* master_end, const std::string &label) {
    int i;
    
    // this container only allow one copy of a value, and we'll have aliasing
    if ( sizeof(value_T) < 2)
        return;

    testContainerType testMap;
    
    auto master_ptr = master_begin;
    double masterSum = 0.0;
    
    // insert the items and sum for comparison
    while (master_ptr != master_end) {
        testMap[ *master_ptr ] = *master_ptr;
        masterSum += *master_ptr;
        master_ptr++;
        }
    
    
    start_timer();
    
    for (i = 0; i < iterations; ++i) {
        double testSum = 0.0;
        auto first = testMap.cubegin();
        auto last = testMap.cuend();
        
        testSum = myaccumulate_pair(first,last,testSum);
        
        if (testSum != masterSum)
            printf("test %i failed\n", current_test);
    }
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );

}

/******************************************************************************/
/******************************************************************************/

// How many items to hold on to for block creation/deletion,
// so memory usage remains reasonable.
constexpr size_t deletionBlockSize(size_t count, size_t itemSize) {
    size_t max = 20*1024*1024*1024ULL;      // 20 gig
    size_t limit = max/(count*itemSize);
    limit = std::min( limit, size_t(1000) );
    limit = std::max( limit, size_t(4) );
    return limit;
}

/******************************************************************************/

template <typename value_T, class testContainerType, bool removeOverhead>
void test_pushback(const value_T* master_begin, const value_T* master_end, const std::string &label) {
    int i, k;

    double overhead = 0.0;

    if (removeOverhead) {
        // first, measure allocation overhead (usually very small)
        start_timer();
        
        for (i = 0; i < iterations; ++i)
            {
            testContainerType *myVector = new testContainerType;
            
            // TODO - ccox - will any compilers optimize this away?
            myVector->push_back( *master_begin );

            delete myVector;
            }
        
        overhead = timer();
    }

    size_t count = master_end - master_begin;
    size_t kDeletionBlockSize = deletionBlockSize(count, sizeof(value_T));

    // lists to be deleted
    std::vector< testContainerType * > holdForDeletion;
    holdForDeletion.resize(kDeletionBlockSize);
    
    double insertTimerAccumulator = 0.0;
    
    
    for (k = 0; k < iterations; k += kDeletionBlockSize) {
    
        int iterationEnd = kDeletionBlockSize;
        if ((k + kDeletionBlockSize) >= iterations)
            iterationEnd = iterations - k;
        
        
        // time the allocation and insertion
        start_timer();
        
        for (i = 0; i < iterationEnd; ++i) {
            testContainerType *myVector = new testContainerType;
            const value_T *master_ptr = master_begin;
            
            while (master_ptr != master_end)
                myVector->push_back( *master_ptr++ );
            
            holdForDeletion[i] = myVector;
            }
        
        insertTimerAccumulator += timer();
        
        
        // delete (not timed)
        for (i = 0; i < iterationEnd; ++i) {
            delete holdForDeletion[i];
            holdForDeletion[i] = NULL;
            }

        }
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    // and subtract the allocation overhead
    record_result( insertTimerAccumulator - overhead, gLabels.back().c_str() );

}

/******************************************************************************/
/******************************************************************************/

template <typename value_T, class testContainerType, bool removeOverhead>
void test_pushfront(const value_T* master_begin, const value_T* master_end, const std::string &label) {
    int i, k;

    double overhead = 0.0;

    if (removeOverhead) {
    
        // first, measure allocation overhead (usually very small)
        start_timer();
        
        for (i = 0; i < iterations; ++i)
            {
            testContainerType *myVector = new testContainerType;
            
            // TODO - ccox - will any compilers optimize this away?
            myVector->push_front( *master_begin );

            delete myVector;
            }
        
        overhead = timer();
        }

    size_t count = master_end - master_begin;
    size_t kDeletionBlockSize = deletionBlockSize(count, sizeof(value_T));

    // lists to be deleted
    std::vector< testContainerType * > holdForDeletion;
    holdForDeletion.resize(kDeletionBlockSize);
    
    double insertTimerAccumulator = 0.0;
    
    
    for (k = 0; k < iterations; k += kDeletionBlockSize) {
    
        int iterationEnd = kDeletionBlockSize;
        if ((k + kDeletionBlockSize) >= iterations)
            iterationEnd = iterations - k;
        
        
        // time the allocation and insertion
        start_timer();
        
        for (i = 0; i < iterationEnd; ++i) {
            testContainerType *myVector = new testContainerType;
            const value_T *master_ptr = master_begin;
            
            while (master_ptr != master_end)
                myVector->push_front( *master_ptr++ );
            
            holdForDeletion[i] = myVector;
            }
        
        insertTimerAccumulator += timer();
        
        
        // delete (not timed)
        for (i = 0; i < iterationEnd; ++i) {
            delete holdForDeletion[i];
            holdForDeletion[i] = NULL;
            }

        }
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    // and subtract the allocation overhead
    record_result( insertTimerAccumulator - overhead, gLabels.back().c_str() );

}

/******************************************************************************/
/******************************************************************************/

template<typename value_T, class testContainerType, bool removeOverhead >
void test_insert_set1(const value_T* master_begin, const value_T* master_end, const std::string &label) {
    int i, k;

    double overhead = 0.0;

    if (removeOverhead) {

        // first, measure allocation overhead (usually very small)
        start_timer();
        
        for (i = 0; i < iterations; ++i)
            {
            testContainerType *mySet = new testContainerType;
            
            // TODO - ccox - will any compilers optimize this away?
            mySet->insert( *master_begin );

            delete mySet;
            }
        
        overhead = timer();
        }

    size_t count = master_end - master_begin;
    size_t kDeletionBlockSize = deletionBlockSize(count, sizeof(value_T));

    // lists to be deleted
    std::vector< testContainerType * > holdForDeletion;
    holdForDeletion.resize(kDeletionBlockSize);
    
    double insertTimerAccumulator = 0.0;
    
    for (k = 0; k < iterations; k += kDeletionBlockSize) {
    
        int iterationEnd = kDeletionBlockSize;
        if ((k + kDeletionBlockSize) >= iterations)
            iterationEnd = iterations - k;
        
        
        // time the allocation and insertion
        start_timer();
        
        for (i = 0; i < iterationEnd; ++i) {
            testContainerType *myVector = new testContainerType;
            const value_T *master_ptr = master_begin;
            
            while (master_ptr != master_end)
                myVector->insert( *master_ptr++ );
            
            holdForDeletion[i] = myVector;
            }
        
        insertTimerAccumulator += timer();
        
        
        // delete (not timed)
        for (i = 0; i < iterationEnd; ++i) {
            delete holdForDeletion[i];
            holdForDeletion[i] = NULL;
            }

        }
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    // and subtract the allocation overhead
    record_result( insertTimerAccumulator - overhead, gLabels.back().c_str() );

}

/******************************************************************************/

template<typename value_T, class testContainerType, bool removeOverhead >
void test_insert_map(const value_T* master_begin, const value_T* master_end, const std::string &label) {
    int i, k;

    double overhead = 0.0;

    if (removeOverhead) {
    
        // first, measure allocation overhead (usually very small)
        start_timer();
        
        for (i = 0; i < iterations; ++i) {
            testContainerType *myMap = new testContainerType;
            
            // TODO - ccox - will any compilers optimize this away?
            (*myMap)[ *master_begin ] = *master_begin;

            delete myMap;
        }
        
        overhead = timer();
    }

    size_t count = master_end - master_begin;
    size_t kDeletionBlockSize = deletionBlockSize(count, sizeof(value_T));

    // lists to be deleted
    std::vector< testContainerType * > holdForDeletion;
    holdForDeletion.resize(kDeletionBlockSize);
    
    double insertTimerAccumulator = 0.0;
    
    for (k = 0; k < iterations; k += kDeletionBlockSize) {
    
        int iterationEnd = kDeletionBlockSize;
        if ((k + kDeletionBlockSize) >= iterations)
            iterationEnd = iterations - k;
        
        
        // time the allocation and insertion
        start_timer();
        
        for (i = 0; i < iterationEnd; ++i) {
            testContainerType *myVector = new testContainerType;
            const value_T *master_ptr = master_begin;
            
            while (master_ptr != master_end) {
                (*myVector)[ *master_ptr ] = *master_ptr;
                master_ptr++;
            }
            
            holdForDeletion[i] = myVector;
            }
        
        insertTimerAccumulator += timer();
        
        
        // delete (not timed)
        for (i = 0; i < iterationEnd; ++i) {
            delete holdForDeletion[i];
            holdForDeletion[i] = NULL;
            }

        }
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    // and subtract the allocation overhead
    record_result( insertTimerAccumulator - overhead, gLabels.back().c_str() );

}

/******************************************************************************/

template<typename value_T, class testContainerType, bool removeOverhead >
void test_insert_multimap(const value_T* master_begin, const value_T* master_end, const std::string &label) {
    int i, k;

    double overhead = 0.0;

    if (removeOverhead) {
    
        // first, measure allocation overhead (usually very small)
        start_timer();
        
        for (i = 0; i < iterations; ++i) {
            testContainerType *myMap = new testContainerType;
            
            // TODO - ccox - will any compilers optimize this away?
            myMap->insert( std::pair<value_T,value_T>(*master_begin,*master_begin) );

            delete myMap;
        }
        
        overhead = timer();
    }

    size_t count = master_end - master_begin;
    size_t kDeletionBlockSize = deletionBlockSize(count, sizeof(value_T));

    // lists to be deleted
    std::vector< testContainerType * > holdForDeletion;
    holdForDeletion.resize(kDeletionBlockSize);
    
    double insertTimerAccumulator = 0.0;
    
    for (k = 0; k < iterations; k += kDeletionBlockSize) {
    
        int iterationEnd = kDeletionBlockSize;
        if ((k + kDeletionBlockSize) >= iterations)
            iterationEnd = iterations - k;
        
        
        // time the allocation and insertion
        start_timer();
        
        for (i = 0; i < iterationEnd; ++i) {
            testContainerType *myVector = new testContainerType;
            const value_T *master_ptr = master_begin;
            
            while (master_ptr != master_end) {
                myVector->insert( std::pair<value_T,value_T>(*master_ptr,*master_ptr) );
                master_ptr++;
            }
            
            holdForDeletion[i] = myVector;
        }
        
        insertTimerAccumulator += timer();
        
        
        // delete (not timed)
        for (i = 0; i < iterationEnd; ++i) {
            delete holdForDeletion[i];
            holdForDeletion[i] = NULL;
            }

        }
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    // and subtract the allocation overhead
    record_result( insertTimerAccumulator - overhead, gLabels.back().c_str() );

}

/******************************************************************************/
/******************************************************************************/

template <typename value_T, class testContainerType>
void test_delete_pushback(const value_T* master_begin, const value_T* master_end, const std::string &label) {
    int i, k;

    size_t count = master_end - master_begin;
    size_t kDeletionBlockSize = deletionBlockSize(count, sizeof(value_T));

    // lists to be deleted
    std::vector< testContainerType * > holdForDeletion;
    holdForDeletion.resize(kDeletionBlockSize);
    
    double deleteTimerAccumulator = 0.0;
    
    
    for (k = 0; k < iterations; k += kDeletionBlockSize) {
    
        int iterationEnd = kDeletionBlockSize;
        if ((k + kDeletionBlockSize) >= iterations)
            iterationEnd = iterations - k;
        
        
        for (i = 0; i < iterationEnd; ++i) {
            testContainerType *myVector = new testContainerType;
            const value_T *master_ptr = master_begin;
            
            while (master_ptr != master_end)
                myVector->push_back( *master_ptr++ );
            
            holdForDeletion[i] = myVector;
            }
        
        
        // time the deletion
        start_timer();
        
        // delete
        for (i = 0; i < iterationEnd; ++i) {
            delete holdForDeletion[i];
            holdForDeletion[i] = NULL;
            }
            
        deleteTimerAccumulator += timer();

        }
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    // and subtract the allocation overhead
    record_result( deleteTimerAccumulator, gLabels.back().c_str() );

}

/******************************************************************************/

template <typename value_T, class testContainerType>
void test_delete_forward(const value_T* master_begin, const value_T* master_end, const std::string &label) {
    int i, k;

    size_t count = master_end - master_begin;
    size_t kDeletionBlockSize = deletionBlockSize(count, sizeof(value_T));

    // lists to be deleted
    std::vector< testContainerType * > holdForDeletion;
    holdForDeletion.resize(kDeletionBlockSize);
    
    double deleteTimerAccumulator = 0.0;
    
    
    for (k = 0; k < iterations; k += kDeletionBlockSize) {
    
        int iterationEnd = kDeletionBlockSize;
        if ((k + kDeletionBlockSize) >= iterations)
            iterationEnd = iterations - k;
        
        for (i = 0; i < iterationEnd; ++i) {
            testContainerType *myVector = new testContainerType;
            const value_T *master_ptr = master_begin;
            
            while (master_ptr != master_end)
                myVector->push_front( *master_ptr++ );

            holdForDeletion[i] = myVector;
            }
        
        // time the deletion
        start_timer();
        
        // delete
        for (i = 0; i < iterationEnd; ++i) {
            delete holdForDeletion[i];
            holdForDeletion[i] = NULL;
            }
            
        deleteTimerAccumulator += timer();

        }
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    // and subtract the allocation overhead
    record_result( deleteTimerAccumulator, gLabels.back().c_str() );

}

/******************************************************************************/

template<typename value_T, class testContainerType >
void test_delete_set1(const value_T* master_begin, const value_T* master_end, const std::string &label) {
    int i, k;

    size_t count = master_end - master_begin;
    size_t kDeletionBlockSize = deletionBlockSize(count, sizeof(value_T));

    // lists to be deleted
    std::vector< testContainerType * > holdForDeletion;
    holdForDeletion.resize(kDeletionBlockSize);
    
    double deleteTimerAccumulator = 0.0;
    
    for (k = 0; k < iterations; k += kDeletionBlockSize) {
    
        int iterationEnd = kDeletionBlockSize;
        if ((k + kDeletionBlockSize) >= iterations)
            iterationEnd = iterations - k;
        
        for (i = 0; i < iterationEnd; ++i) {
            testContainerType *myVector = new testContainerType;
            const value_T *master_ptr = master_begin;
            
            while (master_ptr != master_end)
                myVector->insert( *master_ptr++ );
            
            holdForDeletion[i] = myVector;
            }
        
        // time the deletion
        start_timer();
        
        // delete
        for (i = 0; i < iterationEnd; ++i) {
            delete holdForDeletion[i];
            holdForDeletion[i] = NULL;
            }
        
        deleteTimerAccumulator += timer();

        }
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    // and subtract the allocation overhead
    record_result( deleteTimerAccumulator, gLabels.back().c_str() );

}

/******************************************************************************/

template<typename value_T, class testContainerType >
void test_delete_map(const value_T* master_begin, const value_T* master_end, const std::string &label) {
    int i, k;

    size_t count = master_end - master_begin;
    size_t kDeletionBlockSize = deletionBlockSize(count, sizeof(value_T));

    // lists to be deleted
    std::vector< testContainerType * > holdForDeletion;
    holdForDeletion.resize(kDeletionBlockSize);
    
    double deleteTimerAccumulator = 0.0;
    
    for (k = 0; k < iterations; k += kDeletionBlockSize) {
    
        int iterationEnd = kDeletionBlockSize;
        if ((k + kDeletionBlockSize) >= iterations)
            iterationEnd = iterations - k;
        
        for (i = 0; i < iterationEnd; ++i) {
            testContainerType *myVector = new testContainerType;
            const value_T *master_ptr = master_begin;
            
            while (master_ptr != master_end) {
                (*myVector)[ *master_ptr ] = *master_ptr;
                master_ptr++;
            }
            
            holdForDeletion[i] = myVector;
            }
        
        
        // time the deletion
        start_timer();
        
        // delete
        for (i = 0; i < iterationEnd; ++i) {
            delete holdForDeletion[i];
            holdForDeletion[i] = NULL;
            }
            
        deleteTimerAccumulator += timer();
        }
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    // and subtract the allocation overhead
    record_result( deleteTimerAccumulator, gLabels.back().c_str() );

}

/******************************************************************************/

template<typename value_T, class testContainerType >
void test_delete_multimap(const value_T* master_begin, const value_T* master_end, const std::string &label) {
    int i, k;

    size_t count = master_end - master_begin;
    size_t kDeletionBlockSize = deletionBlockSize(count, sizeof(value_T));

    // lists to be deleted
    std::vector< testContainerType * > holdForDeletion;
    holdForDeletion.resize(kDeletionBlockSize);
    
    double deleteTimerAccumulator = 0.0;
    
    for (k = 0; k < iterations; k += kDeletionBlockSize) {
    
        int iterationEnd = kDeletionBlockSize;
        if ((k + kDeletionBlockSize) >= iterations)
            iterationEnd = iterations - k;
        
        for (i = 0; i < iterationEnd; ++i) {
            testContainerType *myVector = new testContainerType;
            const value_T *master_ptr = master_begin;
            
            while (master_ptr != master_end) {
                myVector->insert( std::pair<value_T,value_T>(*master_ptr,*master_ptr) );
                master_ptr++;
            }
            
            holdForDeletion[i] = myVector;
            }
        
        
        // time the deletion
        start_timer();
        
        // delete
        for (i = 0; i < iterationEnd; ++i) {
            delete holdForDeletion[i];
            holdForDeletion[i] = NULL;
            }
            
        deleteTimerAccumulator += timer();
        }
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    // and subtract the allocation overhead
    record_result( deleteTimerAccumulator, gLabels.back().c_str() );

}

/******************************************************************************/
/******************************************************************************/

template <typename value_T, class testContainerType>
void test_eraseall_pushback(const value_T* master_begin, const value_T* master_end, const std::string &label) {
    int i, k;

    size_t count = master_end - master_begin;
    size_t kDeletionBlockSize = deletionBlockSize(count, sizeof(value_T));

    // lists to be deleted
    std::vector< testContainerType * > holdForDeletion;
    holdForDeletion.resize(kDeletionBlockSize);
    
    double eraseTimerAccumulator = 0.0;
    
    for (k = 0; k < iterations; k += kDeletionBlockSize) {
    
        int iterationEnd = kDeletionBlockSize;
        if ((k + kDeletionBlockSize) >= iterations)
            iterationEnd = iterations - k;
        
        
        for (i = 0; i < iterationEnd; ++i) {
            testContainerType *myVector = new testContainerType;
            const value_T *master_ptr = master_begin;
            
            while (master_ptr != master_end)
                myVector->push_back( *master_ptr++ );
            
            holdForDeletion[i] = myVector;
            }
        
        
        // time the erase
        start_timer();
        
        // erase entries
        for (i = 0; i < iterationEnd; ++i) {
            testContainerType *myVector = holdForDeletion[i];
            auto first = myVector->begin();
            auto last = myVector->end();
            myVector->erase( first, last );
            }
            
        eraseTimerAccumulator += timer();
        
        
        // delete
        for (i = 0; i < iterationEnd; ++i) {
            delete holdForDeletion[i];
            holdForDeletion[i] = NULL;
            }

        }
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( eraseTimerAccumulator, gLabels.back().c_str() );

}

/******************************************************************************/

template<typename value_T, class testContainerType >
void test_eraseall_set1(const value_T* master_begin, const value_T* master_end, const std::string &label) {
    int i, k;

    size_t count = master_end - master_begin;
    size_t kDeletionBlockSize = deletionBlockSize(count, sizeof(value_T));

    // lists to be deleted
    std::vector< testContainerType * > holdForDeletion;
    holdForDeletion.resize(kDeletionBlockSize);
    
    double eraseTimerAccumulator = 0.0;
    
    for (k = 0; k < iterations; k += kDeletionBlockSize) {
    
        int iterationEnd = kDeletionBlockSize;
        if ((k + kDeletionBlockSize) >= iterations)
            iterationEnd = iterations - k;
        
        for (i = 0; i < iterationEnd; ++i) {
            testContainerType *myVector = new testContainerType;
            const value_T *master_ptr = master_begin;
            
            while (master_ptr != master_end)
                myVector->insert( *master_ptr++ );
            
            holdForDeletion[i] = myVector;
            }
        
        
        // time the erase
        start_timer();
        
        // erase entries
        for (i = 0; i < iterationEnd; ++i) {
            testContainerType *myVector = holdForDeletion[i];
            auto first = myVector->begin();
            auto last = myVector->end();
            myVector->erase( first, last );
            }
            
        eraseTimerAccumulator += timer();
        
        
        // delete
        for (i = 0; i < iterationEnd; ++i) {
            delete holdForDeletion[i];
            holdForDeletion[i] = NULL;
            }
        }
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( eraseTimerAccumulator, gLabels.back().c_str() );

}

/******************************************************************************/

template<typename value_T, class testContainerType >
void test_eraseall_map(const value_T* master_begin, const value_T* master_end, const std::string &label) {
    int i, k;

    size_t count = master_end - master_begin;
    size_t kDeletionBlockSize = deletionBlockSize(count, sizeof(value_T));

    // lists to be deleted
    std::vector< testContainerType * > holdForDeletion;
    holdForDeletion.resize(kDeletionBlockSize);
    
    double eraseTimerAccumulator = 0.0;
    
    for (k = 0; k < iterations; k += kDeletionBlockSize) {
    
        int iterationEnd = kDeletionBlockSize;
        if ((k + kDeletionBlockSize) >= iterations)
            iterationEnd = iterations - k;
        
        for (i = 0; i < iterationEnd; ++i) {
            testContainerType *myVector = new testContainerType;
            const value_T *master_ptr = master_begin;
            
            while (master_ptr != master_end) {
                (*myVector)[ *master_ptr ] = *master_ptr;
                master_ptr++;
                }
            
            holdForDeletion[i] = myVector;
            }
        
        
        // time the erase
        start_timer();
        
        // erase entries
        for (i = 0; i < iterationEnd; ++i) {
            testContainerType *myVector = holdForDeletion[i];
            auto first = myVector->begin();
            auto last = myVector->end();
            myVector->erase( first, last );
            }
            
        eraseTimerAccumulator += timer();
        
        
        // delete
        for (i = 0; i < iterationEnd; ++i) {
            delete holdForDeletion[i];
            holdForDeletion[i] = NULL;
            }
        }
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( eraseTimerAccumulator, gLabels.back().c_str() );
}

/******************************************************************************/

template<typename value_T, class testContainerType >
void test_eraseall_multimap(const value_T* master_begin, const value_T* master_end, const std::string &label) {
    int i, k;

    size_t count = master_end - master_begin;
    size_t kDeletionBlockSize = deletionBlockSize(count, sizeof(value_T));

    // lists to be deleted
    std::vector< testContainerType * > holdForDeletion;
    holdForDeletion.resize(kDeletionBlockSize);
    
    double eraseTimerAccumulator = 0.0;
    
    for (k = 0; k < iterations; k += kDeletionBlockSize) {
    
        int iterationEnd = kDeletionBlockSize;
        if ((k + kDeletionBlockSize) >= iterations)
            iterationEnd = iterations - k;
        
        for (i = 0; i < iterationEnd; ++i) {
            testContainerType *myVector = new testContainerType;
            const value_T *master_ptr = master_begin;
            
            while (master_ptr != master_end) {
                myVector->insert( std::pair<value_T,value_T>(*master_ptr,*master_ptr) );
                master_ptr++;
                }
            
            holdForDeletion[i] = myVector;
            }
        
        
        // time the erase
        start_timer();
        
        // erase entries
        for (i = 0; i < iterationEnd; ++i) {
            testContainerType *myVector = holdForDeletion[i];
            auto first = myVector->begin();
            auto last = myVector->end();
            myVector->erase( first, last );
            }
            
        eraseTimerAccumulator += timer();
        
        
        // delete
        for (i = 0; i < iterationEnd; ++i) {
            delete holdForDeletion[i];
            holdForDeletion[i] = NULL;
            }
        }
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( eraseTimerAccumulator, gLabels.back().c_str() );

}

/******************************************************************************/
/******************************************************************************/

template <typename value_T, class testContainerType>
void test_clearall_pushback(const value_T* master_begin, const value_T* master_end, const std::string &label) {
    int i, k;

    size_t count = master_end - master_begin;
    size_t kDeletionBlockSize = deletionBlockSize(count, sizeof(value_T));

    // lists to be deleted
    std::vector< testContainerType * > holdForDeletion;
    holdForDeletion.resize(kDeletionBlockSize);
    
    double eraseTimerAccumulator = 0.0;
    
    for (k = 0; k < iterations; k += kDeletionBlockSize) {
    
        int iterationEnd = kDeletionBlockSize;
        if ((k + kDeletionBlockSize) >= iterations)
            iterationEnd = iterations - k;
        
        
        for (i = 0; i < iterationEnd; ++i) {
            testContainerType *myVector = new testContainerType;
            const value_T *master_ptr = master_begin;
            
            while (master_ptr != master_end)
                myVector->push_back( *master_ptr++ );
            
            holdForDeletion[i] = myVector;
            }
        
        
        // time the erase
        start_timer();
        
        // erase entries
        for (i = 0; i < iterationEnd; ++i) {
            testContainerType *myVector = holdForDeletion[i];
            myVector->clear();
            }
        
        eraseTimerAccumulator += timer();
        
        
        // delete
        for (i = 0; i < iterationEnd; ++i) {
            delete holdForDeletion[i];
            holdForDeletion[i] = NULL;
            }

        }
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( eraseTimerAccumulator, gLabels.back().c_str() );

}
/******************************************************************************/

template <typename value_T, class testContainerType>
void test_clearall_forward(const value_T* master_begin, const value_T* master_end, const std::string &label) {
    int i, k;

    size_t count = master_end - master_begin;
    size_t kDeletionBlockSize = deletionBlockSize(count, sizeof(value_T));

    // lists to be deleted
    std::vector< testContainerType * > holdForDeletion;
    holdForDeletion.resize(kDeletionBlockSize);
    
    double eraseTimerAccumulator = 0.0;
    
    for (k = 0; k < iterations; k += kDeletionBlockSize) {
    
        int iterationEnd = kDeletionBlockSize;
        if ((k + kDeletionBlockSize) >= iterations)
            iterationEnd = iterations - k;
        
        
        for (i = 0; i < iterationEnd; ++i) {
            testContainerType *myVector = new testContainerType;
            const value_T *master_ptr = master_begin;
            
            while (master_ptr != master_end)
                myVector->push_front( *master_ptr++ );
            
            holdForDeletion[i] = myVector;
            }
        
        
        // time the erase
        start_timer();
        
        // erase entries
        for (i = 0; i < iterationEnd; ++i) {
            testContainerType *myVector = holdForDeletion[i];
            myVector->clear();
            }
        
        eraseTimerAccumulator += timer();
        
        
        // delete
        for (i = 0; i < iterationEnd; ++i) {
            delete holdForDeletion[i];
            holdForDeletion[i] = NULL;
            }

        }
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( eraseTimerAccumulator, gLabels.back().c_str() );

}

/******************************************************************************/

template<typename value_T, class testContainerType >
void test_clearall_set1(const value_T* master_begin, const value_T* master_end, const std::string &label) {
    int i, k;

    size_t count = master_end - master_begin;
    size_t kDeletionBlockSize = deletionBlockSize(count, sizeof(value_T));

    // lists to be deleted
    std::vector< testContainerType * > holdForDeletion;
    holdForDeletion.resize(kDeletionBlockSize);
    
    double eraseTimerAccumulator = 0.0;
    
    for (k = 0; k < iterations; k += kDeletionBlockSize) {
    
        int iterationEnd = kDeletionBlockSize;
        if ((k + kDeletionBlockSize) >= iterations)
            iterationEnd = iterations - k;
        
        for (i = 0; i < iterationEnd; ++i) {
            testContainerType *myVector = new testContainerType;
            const value_T *master_ptr = master_begin;
            
            while (master_ptr != master_end)
                myVector->insert( *master_ptr++ );
            
            holdForDeletion[i] = myVector;
            }
        
        
        // time the erase
        start_timer();
        
        // erase entries
        for (i = 0; i < iterationEnd; ++i) {
            testContainerType *myVector = holdForDeletion[i];
            myVector->clear();
            }
            
        eraseTimerAccumulator += timer();
        
        
        // delete
        for (i = 0; i < iterationEnd; ++i) {
            delete holdForDeletion[i];
            holdForDeletion[i] = NULL;
            }
        }
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( eraseTimerAccumulator, gLabels.back().c_str() );

}

/******************************************************************************/

template<typename value_T, class testContainerType>
void test_clearall_map(const value_T* master_begin, const value_T* master_end, const std::string &label) {
    int i, k;

    size_t count = master_end - master_begin;
    size_t kDeletionBlockSize = deletionBlockSize(count, sizeof(value_T));

    // lists to be deleted
    std::vector< testContainerType * > holdForDeletion;
    holdForDeletion.resize(kDeletionBlockSize);
    
    double eraseTimerAccumulator = 0.0;
    
    for (k = 0; k < iterations; k += kDeletionBlockSize) {
    
        int iterationEnd = kDeletionBlockSize;
        if ((k + kDeletionBlockSize) >= iterations)
            iterationEnd = iterations - k;
        
        for (i = 0; i < iterationEnd; ++i) {
            testContainerType *myVector = new testContainerType;
            const value_T *master_ptr = master_begin;
            
            while (master_ptr != master_end) {
                (*myVector)[ *master_ptr ] = *master_ptr;
                master_ptr++;
            }
            
            holdForDeletion[i] = myVector;
            }
        
        
        // time the erase
        start_timer();
        
        // erase entries
        for (i = 0; i < iterationEnd; ++i) {
            testContainerType *myVector = holdForDeletion[i];
            myVector->clear();
            }
            
        eraseTimerAccumulator += timer();
        
        
        // delete
        for (i = 0; i < iterationEnd; ++i) {
            delete holdForDeletion[i];
            holdForDeletion[i] = NULL;
            }
        }
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( eraseTimerAccumulator, gLabels.back().c_str() );

}

/******************************************************************************/

template<typename value_T, class testContainerType>
void test_clearall_multimap(const value_T* master_begin, const value_T* master_end, const std::string &label) {
    int i, k;

    size_t count = master_end - master_begin;
    size_t kDeletionBlockSize = deletionBlockSize(count, sizeof(value_T));

    // lists to be deleted
    std::vector< testContainerType * > holdForDeletion;
    holdForDeletion.resize(kDeletionBlockSize);
    
    double eraseTimerAccumulator = 0.0;
    
    for (k = 0; k < iterations; k += kDeletionBlockSize) {
    
        int iterationEnd = kDeletionBlockSize;
        if ((k + kDeletionBlockSize) >= iterations)
            iterationEnd = iterations - k;
        
        for (i = 0; i < iterationEnd; ++i) {
            testContainerType *myVector = new testContainerType;
            const value_T *master_ptr = master_begin;
            
            while (master_ptr != master_end) {
                myVector->insert( std::pair<value_T,value_T>(*master_ptr,*master_ptr) );
                master_ptr++;
            }
            
            holdForDeletion[i] = myVector;
            }
        
        
        // time the erase
        start_timer();
        
        // erase entries
        for (i = 0; i < iterationEnd; ++i) {
            testContainerType *myVector = holdForDeletion[i];
            myVector->clear();
            }
            
        eraseTimerAccumulator += timer();
        
        
        // delete
        for (i = 0; i < iterationEnd; ++i) {
            delete holdForDeletion[i];
            holdForDeletion[i] = NULL;
            }
        }
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( eraseTimerAccumulator, gLabels.back().c_str() );

}

/******************************************************************************/
/******************************************************************************/

template <typename value_T, class testContainerType>
void test_popfront(const value_T* master_begin, const value_T* master_end, const std::string &label) {
    int i, k;

    size_t count = master_end - master_begin;
    size_t kDeletionBlockSize = deletionBlockSize(count, sizeof(value_T));

    // lists to be deleted
    std::vector< testContainerType * > holdForDeletion;
    holdForDeletion.resize(kDeletionBlockSize);
    
    double eraseTimerAccumulator = 0.0;
    
    for (k = 0; k < iterations; k += kDeletionBlockSize) {
    
        int iterationEnd = kDeletionBlockSize;
        if ((k + kDeletionBlockSize) >= iterations)
            iterationEnd = iterations - k;
        
        for (i = 0; i < iterationEnd; ++i) {
            testContainerType *myVector = new testContainerType;
            const value_T *master_ptr = master_begin;
            
            while (master_ptr != master_end)
                myVector->push_back( *master_ptr++ );
            
            holdForDeletion[i] = myVector;
            }
        
        
        // time the erase
        start_timer();
        
        // erase entries
        for (i = 0; i < iterationEnd; ++i) {
            testContainerType *myVector = holdForDeletion[i];
            
            const value_T *master_ptr = master_begin;
            
            while (master_ptr != master_end) {
                myVector->pop_front();
                ++master_ptr;
                }
            }
        
        eraseTimerAccumulator += timer();
        
        
        // delete
        for (i = 0; i < iterationEnd; ++i) {
            delete holdForDeletion[i];
            holdForDeletion[i] = NULL;
            }

        }
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( eraseTimerAccumulator, gLabels.back().c_str() );
}

/******************************************************************************/

template <typename value_T, class testContainerType>
void test_popfront_forward(const value_T* master_begin, const value_T* master_end, const std::string &label) {
    int i, k;

    size_t count = master_end - master_begin;
    size_t kDeletionBlockSize = deletionBlockSize(count, sizeof(value_T));

    // lists to be deleted
    std::vector< testContainerType * > holdForDeletion;
    holdForDeletion.resize(kDeletionBlockSize);
    
    double eraseTimerAccumulator = 0.0;
    
    for (k = 0; k < iterations; k += kDeletionBlockSize) {
    
        int iterationEnd = kDeletionBlockSize;
        if ((k + kDeletionBlockSize) >= iterations)
            iterationEnd = iterations - k;
        
        for (i = 0; i < iterationEnd; ++i) {
            testContainerType *myVector = new testContainerType;
            const value_T *master_ptr = master_begin;
            
            while (master_ptr != master_end)
                myVector->push_front( *master_ptr++ );

            holdForDeletion[i] = myVector;
            }
        
        // time the erase
        start_timer();
        
        // erase entries
        for (i = 0; i < iterationEnd; ++i) {
            testContainerType *myVector = holdForDeletion[i];
            
            const value_T *master_ptr = master_begin;
            
            while (master_ptr != master_end) {
                myVector->pop_front();
                ++master_ptr;
                }
            }
        
        eraseTimerAccumulator += timer();
        
        
        // delete
        for (i = 0; i < iterationEnd; ++i) {
            delete holdForDeletion[i];
            holdForDeletion[i] = NULL;
            }

        }
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( eraseTimerAccumulator, gLabels.back().c_str() );

}

/******************************************************************************/
/******************************************************************************/

template <typename value_T, class testContainerType>
void test_popback(const value_T* master_begin, const value_T* master_end, const std::string &label) {
    int i, k;

    size_t count = master_end - master_begin;
    size_t kDeletionBlockSize = deletionBlockSize(count, sizeof(value_T));

    // lists to be deleted
    std::vector< testContainerType * > holdForDeletion;
    holdForDeletion.resize(kDeletionBlockSize);
    
    double eraseTimerAccumulator = 0.0;
    
    for (k = 0; k < iterations; k += kDeletionBlockSize) {
    
        int iterationEnd = kDeletionBlockSize;
        if ((k + kDeletionBlockSize) >= iterations)
            iterationEnd = iterations - k;
        
        for (i = 0; i < iterationEnd; ++i) {
            testContainerType *myVector = new testContainerType;
            const value_T *master_ptr = master_begin;
            
            while (master_ptr != master_end)
                myVector->push_back( *master_ptr++ );
            
            holdForDeletion[i] = myVector;
            }
        
        
        // time the erase
        start_timer();
        
        // erase entries
        for (i = 0; i < iterationEnd; ++i) {
            testContainerType *myVector = holdForDeletion[i];
            
            const value_T *master_ptr = master_begin;
            
            while (master_ptr != master_end) {
                myVector->pop_back();
                ++master_ptr;
                }
            }
        
        eraseTimerAccumulator += timer();
        
        
        // delete
        for (i = 0; i < iterationEnd; ++i) {
            delete holdForDeletion[i];
            holdForDeletion[i] = NULL;
            }

        }
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( eraseTimerAccumulator, gLabels.back().c_str() );

}

/******************************************************************************/
/******************************************************************************/

template<typename value_T, class testContainerType>
void test_find_set1(const value_T* data_begin, const value_T* data_end, const value_T *lookup_begin, const value_T *lookup_end, const std::string &label) {
    int i;
    
    double masterSum = 0.0;
    
    testContainerType myVector;
    const value_T *master_ptr = data_begin;
    
    // insert the items and sum for comparison
    while (master_ptr != data_end) {
        myVector.insert( *master_ptr );
        masterSum += *master_ptr;
        master_ptr++;
        }
    
    
    // time the lookup and sum
    start_timer();
    
    for (i = 0; i < iterations; ++i) {
        const value_T *lookup_ptr = lookup_begin;
        double testSum = 0.0;
        
        while (lookup_ptr != lookup_end) {
            auto item = myVector.find( *lookup_ptr++ );
            testSum = testSum + *item;
            }
        
        if (testSum != masterSum)
            printf("test %i failed\n", current_test);
        }
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );

}

/******************************************************************************/

template<typename value_T, class testContainerType>
void test_find_map(const value_T* data_begin, const value_T* data_end, const value_T *lookup_begin, const value_T *lookup_end, const std::string &label) {
    int i;
    
    double masterSum = 0.0;
    
    testContainerType myVector;
    const value_T *master_ptr = data_begin;
    
    // insert the items and sum for comparison
    while (master_ptr != data_end) {
        myVector[ *master_ptr ] = *master_ptr;
        masterSum += *master_ptr;
        master_ptr++;
        }
    
    
    // time the lookup and sum
    start_timer();
    
    for (i = 0; i < iterations; ++i) {
        const value_T *lookup_ptr = lookup_begin;
        double testSum = 0.0;
        
        while (lookup_ptr != lookup_end) {
            auto item = myVector.find( *lookup_ptr++ );
            testSum = testSum + (*item).second;
            }
        
        if (testSum != masterSum)
            printf("test %i failed\n", current_test);
        }
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );

}

/******************************************************************************/

template<typename value_T, class testContainerType>
void test_find_multimap(const value_T* data_begin, const value_T* data_end, const value_T *lookup_begin, const value_T *lookup_end, const std::string &label) {
    int i;
    
    double masterSum = 0.0;
    
    testContainerType myVector;
    const value_T *master_ptr = data_begin;
    
    // insert the items and sum for comparison
    while (master_ptr != data_end) {
       myVector.insert( std::pair<value_T,value_T>(*master_ptr,*master_ptr) );
        masterSum += *master_ptr;
        master_ptr++;
        }
    
    
    // time the lookup and sum
    start_timer();
    
    for (i = 0; i < iterations; ++i) {
        const value_T *lookup_ptr = lookup_begin;
        double testSum = 0.0;
        
        while (lookup_ptr != lookup_end) {
            auto item = myVector.find( *lookup_ptr++ );
            testSum = testSum + (*item).second;
            }
        
        if (testSum != masterSum)
            printf("test %i failed\n", current_test);
        }
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );

}

/******************************************************************************/

template<typename value_T, class testContainerType>
void test_find_simplehashmap(const value_T* data_begin, const value_T* data_end, const value_T *lookup_begin, const value_T *lookup_end, const std::string &label) {
    int i;
    
    double masterSum = 0.0;
    
    testContainerType myVector;
    const value_T *master_ptr = data_begin;
    
    // insert the items and sum for comparison
    while (master_ptr != data_end) {
        myVector.insert( *master_ptr, *master_ptr );
        masterSum += *master_ptr;
        master_ptr++;
        }
    
    
    // time the lookup and sum
    start_timer();
    
    for (i = 0; i < iterations; ++i) {
        const value_T *lookup_ptr = lookup_begin;
        double testSum = 0.0;
        
        while (lookup_ptr != lookup_end) {
            auto item = myVector.find( *lookup_ptr++ );
            testSum = testSum + *item;
            }
        
        if (testSum != masterSum)
            printf("test %i failed\n", current_test);
        }
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );

}

/******************************************************************************/

template<typename value_T, class testContainerType>
void test_find_pushback(const value_T* data_begin, const value_T* data_end, const value_T *lookup_begin, const value_T *lookup_end, const std::string &label) {
    int i;
    
    double masterSum = 0.0;
    
    testContainerType myVector;
    const value_T *master_ptr = data_begin;
    
    // insert the items and sum for comparison
    while (master_ptr != data_end) {
        myVector.push_back( *master_ptr );
        masterSum += *master_ptr;
        master_ptr++;
        }
    
    
    // time the lookup and sum
    start_timer();
    
    for (i = 0; i < iterations; ++i) {
        const value_T *lookup_ptr = lookup_begin;
        double testSum = 0.0;
        
        while (lookup_ptr != lookup_end) {
            auto item = std::find( myVector.cbegin(), myVector.cend(), *lookup_ptr++ );
            testSum = testSum + *item;
            }
        
        if (testSum != masterSum)
            printf("test %i failed\n", current_test);
        }
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );

}

/******************************************************************************/

template<typename value_T, class testContainerType>
void test_find_pushback_forward(const value_T* data_begin, const value_T* data_end, const value_T *lookup_begin, const value_T *lookup_end, const std::string &label) {
    int i;
    
    double masterSum = 0.0;
    
    testContainerType myVector;
    const value_T *master_ptr = data_begin;
    
    // insert the items and sum for comparison
    while (master_ptr != data_end) {
        myVector.push_front( *master_ptr );
        masterSum += *master_ptr;
        master_ptr++;
        }
    
    // time the lookup and sum
    start_timer();
    
    for (i = 0; i < iterations; ++i) {
        const value_T *lookup_ptr = lookup_begin;
        double testSum = 0.0;
        
        while (lookup_ptr != lookup_end) {
            auto item = std::find( myVector.cbegin(), myVector.cend(), *lookup_ptr++ );
            testSum = testSum + *item;
            }
        
        if (testSum != masterSum)
            printf("test %i failed\n", current_test);
        }
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );

}

/******************************************************************************/

template<typename value_T, class testContainerType>
void test_find_pushback_sorted(const value_T* data_begin, const value_T* data_end, const value_T *lookup_begin, const value_T *lookup_end, const std::string &label) {
    int i;
    
    double masterSum = 0.0;
    
    testContainerType myVector;
    const value_T *master_ptr = data_begin;
    
    // insert the items and sum for comparison
    while (master_ptr != data_end) {
        myVector.push_back( *master_ptr );
        masterSum += *master_ptr;
        master_ptr++;
        }
    
    // sort the list
    std::sort( myVector.begin(), myVector.end() );
    
    // time the lookup and sum
    start_timer();
    
    for (i = 0; i < iterations; ++i) {
        const value_T *lookup_ptr = lookup_begin;
        double testSum = 0.0;
        
        while (lookup_ptr != lookup_end) {
            auto item = std::lower_bound( myVector.cbegin(), myVector.cend(), *lookup_ptr++ );
            testSum = testSum + *item;
            }
        
        if (testSum != masterSum)
            printf("test %i failed\n", current_test);
        }
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( timer(), gLabels.back().c_str() );

}

/******************************************************************************/
/******************************************************************************/

template<typename value_T>
void test_duplicate_array(const value_T* master_begin, const value_T* master_end, const std::string &label) {
    int i, k;

    size_t count = master_end - master_begin;
    size_t kDeletionBlockSize = deletionBlockSize(count, sizeof(value_T));
    
    ptrdiff_t length = master_end - master_begin;

    // lists to be deleted
    std::vector< value_T * > holdForDeletion;
    holdForDeletion.resize(kDeletionBlockSize);
    
    double duplicateTimerAccumulator = 0.0;
    
    for (k = 0; k < iterations; k += kDeletionBlockSize) {
    
        int iterationEnd = kDeletionBlockSize;
        if ((k + kDeletionBlockSize) >= iterations)
            iterationEnd = iterations - k;
        
        
        // time the duplication
        start_timer();
        
        for (i = 0; i < iterationEnd; ++i) {
            value_T *myDuplicate = new value_T[length];
            
            memcpy( myDuplicate, master_begin, length*sizeof(value_T) );
            
            holdForDeletion[i] = myDuplicate;
            }
        
        duplicateTimerAccumulator += timer();
        
        
        // delete (not timed)
        for (i = 0; i < iterationEnd; ++i) {
            delete holdForDeletion[i];
            holdForDeletion[i] = NULL;
            }

        }
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( duplicateTimerAccumulator, gLabels.back().c_str() );
}

/******************************************************************************/

template <class testContainerType>
void test_duplicate_core(testContainerType &master, const size_t count, const std::string &label) {
    int i, k;

    size_t kDeletionBlockSize = deletionBlockSize(count, sizeof(typename testContainerType::value_type));

    // lists to be deleted
    std::vector< testContainerType * > holdForDeletion;
    holdForDeletion.resize(kDeletionBlockSize);
    
    double duplicateTimerAccumulator = 0.0;
    
    for (k = 0; k < iterations; k += kDeletionBlockSize) {
    
        int iterationEnd = kDeletionBlockSize;
        if ((k + kDeletionBlockSize) >= iterations)
            iterationEnd = iterations - k;
        
        
        // time the duplication
        start_timer();
        
        for (i = 0; i < iterationEnd; ++i) {
            testContainerType *myDuplicate = new testContainerType;
            
            *myDuplicate = master;
            
            holdForDeletion[i] = myDuplicate;
            }
        
        duplicateTimerAccumulator += timer();
        
        
        // delete (not timed)
        for (i = 0; i < iterationEnd; ++i) {
            delete holdForDeletion[i];
            holdForDeletion[i] = NULL;
            }

        }
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( duplicateTimerAccumulator, gLabels.back().c_str() );

}

/******************************************************************************/

template <typename value_T, class testContainerType>
void test_duplicate1(const value_T* master_begin, const value_T* master_end, const std::string &label) {
    
    testContainerType master_copy;
    ptrdiff_t length = master_end - master_begin;
    master_copy.resize( length );
    
    std::copy( master_begin, master_end, master_copy.begin() );
    
    test_duplicate_core( master_copy, length, label );
}

/******************************************************************************/

template <typename value_T, class testContainerType>
void test_duplicate2(const value_T* master_begin, const value_T* master_end, const std::string &label) {
    
    testContainerType master_copy;
    
    ptrdiff_t length = master_end - master_begin;
    const value_T *master_ptr = master_begin;
    while (master_ptr != master_end)
        master_copy.push_back( *master_ptr++ );
    
    test_duplicate_core( master_copy, length, label );
}

/******************************************************************************/

template <typename value_T, class testContainerType>
void test_duplicate_set(const value_T* master_begin, const value_T* master_end, const std::string &label) {
    
    testContainerType master_copy;
    
    ptrdiff_t length = master_end - master_begin;
    const value_T *master_ptr = master_begin;
    while (master_ptr != master_end)
        master_copy.insert( *master_ptr++ );
    
    test_duplicate_core( master_copy, length, label );
}

/******************************************************************************/

template <typename value_T, class testContainerType>
void test_duplicate_map(const value_T* master_begin, const value_T* master_end, const std::string &label) {
    
    testContainerType master_copy;
    
    ptrdiff_t length = master_end - master_begin;
    const value_T *master_ptr = master_begin;
    while (master_ptr != master_end) {
        master_copy[ *master_ptr ] = *master_ptr;
        master_ptr++;
        }
    
    test_duplicate_core( master_copy, length, label );
}

/******************************************************************************/

template <typename value_T, class testContainerType>
void test_duplicate_multimap(const value_T* master_begin, const value_T* master_end, const std::string &label) {
    
    testContainerType master_copy;
    
    ptrdiff_t length = master_end - master_begin;
    const value_T *master_ptr = master_begin;
    while (master_ptr != master_end) {
        master_copy.insert( std::pair<value_T,value_T>(*master_ptr,*master_ptr) );
        master_ptr++;
        }
    
    test_duplicate_core( master_copy, length, label );
}

/******************************************************************************/
/******************************************************************************/

template <typename value_T, class testContainerType>
void test_erase_pushback(const value_T* master_begin, const value_T* master_end, const value_T *lookup_begin, const value_T *lookup_end, const std::string &label) {
    int i, k;

    size_t count = master_end - master_begin;
    size_t kDeletionBlockSize = deletionBlockSize(count, sizeof(value_T));

    // lists to be deleted
    std::vector< testContainerType * > holdForDeletion;
    holdForDeletion.resize(kDeletionBlockSize);
    
    double eraseTimerAccumulator = 0.0;
    
    for (k = 0; k < iterations; k += kDeletionBlockSize) {
    
        int iterationEnd = kDeletionBlockSize;
        if ((k + kDeletionBlockSize) >= iterations)
            iterationEnd = iterations - k;
        
        
        for (i = 0; i < iterationEnd; ++i) {
            testContainerType *myVector = new testContainerType;
            const value_T *master_ptr = master_begin;
            
            while (master_ptr != master_end)
                myVector->push_back( *master_ptr++ );
            
            holdForDeletion[i] = myVector;
            }
        
        
        // time the erase
        start_timer();
        
        // erase entries
        for (i = 0; i < iterationEnd; ++i) {
            testContainerType *myVector = holdForDeletion[i];
            const value_T *lookup_ptr = lookup_begin;
            while (lookup_ptr != lookup_end) {
                auto item = std::find( myVector->begin(), myVector->end(), *lookup_ptr++ );
                myVector->erase( item );
                }
            }
            
        eraseTimerAccumulator += timer();
        

        // delete
        for (i = 0; i < iterationEnd; ++i) {
            delete holdForDeletion[i];
            holdForDeletion[i] = NULL;
            }

        }
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( eraseTimerAccumulator, gLabels.back().c_str() );

}

/******************************************************************************/

template <typename value_T, class testContainerType>
void test_erase_pushback_sorted(const value_T* master_begin, const value_T* master_end, const value_T *lookup_begin, const value_T *lookup_end, const std::string &label) {
    int i, k;

    size_t count = master_end - master_begin;
    size_t kDeletionBlockSize = deletionBlockSize(count, sizeof(value_T));

    // lists to be deleted
    std::vector< testContainerType * > holdForDeletion;
    holdForDeletion.resize(kDeletionBlockSize);
    
    double eraseTimerAccumulator = 0.0;
    
    for (k = 0; k < iterations; k += kDeletionBlockSize) {
    
        int iterationEnd = kDeletionBlockSize;
        if ((k + kDeletionBlockSize) >= iterations)
            iterationEnd = iterations - k;
        
        // create one structure, sort, and copy the rest
        {
            testContainerType *myVector = new testContainerType;
            const value_T *master_ptr = master_begin;
            
            while (master_ptr != master_end)
                myVector->push_back( *master_ptr++ );
            
            std::sort( myVector->begin(), myVector->end() );
            
            holdForDeletion[0] = myVector;
        }
        
        for (i = 1; i < iterationEnd; ++i) {
            testContainerType *myVector = new testContainerType;
            *myVector = *holdForDeletion[0]; // copy contents
            holdForDeletion[i] = myVector;
            }
        

        // time the erase
        start_timer();
        
        // erase entries
        for (i = 0; i < iterationEnd; ++i) {
            testContainerType *myVector = holdForDeletion[i];
            const value_T *lookup_ptr = lookup_begin;
            while (lookup_ptr != lookup_end) {
                auto item = std::lower_bound( myVector->cbegin(), myVector->cend(), *lookup_ptr++ );
                myVector->erase( item );
                }
            }
        
        eraseTimerAccumulator += timer();
        

        // delete
        for (i = 0; i < iterationEnd; ++i) {
            delete holdForDeletion[i];
            holdForDeletion[i] = NULL;
            }

        }
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( eraseTimerAccumulator, gLabels.back().c_str() );

}
/******************************************************************************/

template <typename value_T, class testContainerType>
void test_erase_pushback_forward(const value_T* master_begin, const value_T* master_end, const value_T *lookup_begin, const value_T *lookup_end, const std::string &label) {
    int i, k;

    size_t count = master_end - master_begin;
    size_t kDeletionBlockSize = deletionBlockSize(count, sizeof(value_T));

    // lists to be deleted
    std::vector< testContainerType * > holdForDeletion;
    holdForDeletion.resize(kDeletionBlockSize);
    
    double eraseTimerAccumulator = 0.0;
    
    for (k = 0; k < iterations; k += kDeletionBlockSize) {
    
        int iterationEnd = kDeletionBlockSize;
        if ((k + kDeletionBlockSize) >= iterations)
            iterationEnd = iterations - k;
        
        for (i = 0; i < iterationEnd; ++i) {
            testContainerType *myVector = new testContainerType;
            const value_T *master_ptr = master_begin;
            
            while (master_ptr != master_end)
                myVector->push_front( *master_ptr++ );
            
            holdForDeletion[i] = myVector;
            }
        
        
        // time the erase
        start_timer();
        
        // erase entries
        for (i = 0; i < iterationEnd; ++i) {
            testContainerType *myVector = holdForDeletion[i];
            const value_T *lookup_ptr = lookup_begin;
            while (lookup_ptr != lookup_end) {
                myVector->remove( *lookup_ptr++ );
                }
            }
            
        eraseTimerAccumulator += timer();
        

        // delete
        for (i = 0; i < iterationEnd; ++i) {
            delete holdForDeletion[i];
            holdForDeletion[i] = NULL;
            }

        }
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( eraseTimerAccumulator, gLabels.back().c_str() );

}

/******************************************************************************/

template<typename value_T, class testContainerType >
void test_erase_set1(const value_T* master_begin, const value_T* master_end, const value_T *lookup_begin, const value_T *lookup_end, const std::string &label) {
    int i, k;

    size_t count = master_end - master_begin;
    size_t kDeletionBlockSize = deletionBlockSize(count, sizeof(value_T));

    // lists to be deleted
    std::vector< testContainerType * > holdForDeletion;
    holdForDeletion.resize(kDeletionBlockSize);
    
    double eraseTimerAccumulator = 0.0;
    
    for (k = 0; k < iterations; k += kDeletionBlockSize) {
    
        int iterationEnd = kDeletionBlockSize;
        if ((k + kDeletionBlockSize) >= iterations)
            iterationEnd = iterations - k;
        
        for (i = 0; i < iterationEnd; ++i) {
            testContainerType *myVector = new testContainerType;
            const value_T *master_ptr = master_begin;
            
            while (master_ptr != master_end)
                myVector->insert( *master_ptr++ );
            
            holdForDeletion[i] = myVector;
            }
        
        
        // time the erase
        start_timer();
        
        // erase entries
        for (i = 0; i < iterationEnd; ++i) {
            testContainerType *myVector = holdForDeletion[i];
            const value_T *lookup_ptr = lookup_begin;
            while (lookup_ptr != lookup_end)
                myVector->erase( *lookup_ptr++ );
            }
            
        eraseTimerAccumulator += timer();
        
        
        // delete
        for (i = 0; i < iterationEnd; ++i) {
            delete holdForDeletion[i];
            holdForDeletion[i] = NULL;
            }
        }
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( eraseTimerAccumulator, gLabels.back().c_str() );

}

/******************************************************************************/

template<typename value_T, class testContainerType >
void test_erase_map(const value_T* master_begin, const value_T* master_end, const value_T *lookup_begin, const value_T *lookup_end, const std::string &label) {
    int i, k;

    size_t count = master_end - master_begin;
    size_t kDeletionBlockSize = deletionBlockSize(count, sizeof(value_T));

    // lists to be deleted
    std::vector< testContainerType * > holdForDeletion;
    holdForDeletion.resize(kDeletionBlockSize);
    
    double eraseTimerAccumulator = 0.0;
    
    for (k = 0; k < iterations; k += kDeletionBlockSize) {
    
        int iterationEnd = kDeletionBlockSize;
        if ((k + kDeletionBlockSize) >= iterations)
            iterationEnd = iterations - k;
        
        for (i = 0; i < iterationEnd; ++i) {
            testContainerType *myVector = new testContainerType;
            const value_T *master_ptr = master_begin;
            
            while (master_ptr != master_end) {
                (*myVector)[ *master_ptr ] = *master_ptr;
                master_ptr++;
            }
            
            holdForDeletion[i] = myVector;
            }
        
        
        // time the erase
        start_timer();
        
        // erase entries
        for (i = 0; i < iterationEnd; ++i) {
            testContainerType *myVector = holdForDeletion[i];
            const value_T *lookup_ptr = lookup_begin;
            while (lookup_ptr != lookup_end)
                myVector->erase( *lookup_ptr++ );
            }
        
        eraseTimerAccumulator += timer();
        
        
        // delete
        for (i = 0; i < iterationEnd; ++i) {
            delete holdForDeletion[i];
            holdForDeletion[i] = NULL;
            }
        }
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( eraseTimerAccumulator, gLabels.back().c_str() );

}

/******************************************************************************/

template<typename value_T, class testContainerType >
void test_erase_multimap(const value_T* master_begin, const value_T* master_end, const value_T *lookup_begin, const value_T *lookup_end, const std::string &label) {
    int i, k;

    size_t count = master_end - master_begin;
    size_t kDeletionBlockSize = deletionBlockSize(count, sizeof(value_T));

    // lists to be deleted
    std::vector< testContainerType * > holdForDeletion;
    holdForDeletion.resize(kDeletionBlockSize);
    
    double eraseTimerAccumulator = 0.0;
    
    for (k = 0; k < iterations; k += kDeletionBlockSize) {
    
        int iterationEnd = kDeletionBlockSize;
        if ((k + kDeletionBlockSize) >= iterations)
            iterationEnd = iterations - k;
        
        for (i = 0; i < iterationEnd; ++i) {
            testContainerType *myVector = new testContainerType;
            const value_T *master_ptr = master_begin;
            
            while (master_ptr != master_end) {
                myVector->insert( std::pair<value_T,value_T>(*master_ptr,*master_ptr) );
                master_ptr++;
            }
            
            holdForDeletion[i] = myVector;
            }
        
        
        // time the erase
        start_timer();
        
        // erase entries
        for (i = 0; i < iterationEnd; ++i) {
            testContainerType *myVector = holdForDeletion[i];
            const value_T *lookup_ptr = lookup_begin;
            while (lookup_ptr != lookup_end)
                myVector->erase( *lookup_ptr++ );
            }
        
        eraseTimerAccumulator += timer();
        
        
        // delete
        for (i = 0; i < iterationEnd; ++i) {
            delete holdForDeletion[i];
            holdForDeletion[i] = NULL;
            }
        }
    
    // need the labels to remain valid until we print the summary
    gLabels.push_back( label );
    record_result( eraseTimerAccumulator, gLabels.back().c_str() );

}

/******************************************************************************/
/******************************************************************************/

template<typename value_T>
void testCopyEntries(const value_T *master_table, size_t item_count, const std::string &myTypeName, size_t iteration_count, bool do_summarize = true ) {

    iterations = iteration_count;

    test_copy_array(master_table, master_table+item_count, myTypeName + " array copy entries");
    test_copy_stdarray(master_table, master_table+item_count, myTypeName + " std::array copy entries");
    test_copy< value_T, std::vector<value_T> >(master_table, master_table+item_count, myTypeName + " std::vector copy entries");
    test_copy< value_T, std::deque<value_T> >(master_table, master_table+item_count, myTypeName + " std::deque copy entries");
    test_copy< value_T, std::forward_list<value_T> >(master_table, master_table+item_count, myTypeName + " std::forward_list copy entries");
    test_copy< value_T, std::list<value_T> >(master_table, master_table+item_count, myTypeName + " std::list copy entries");
    test_copy< value_T, SingleLinkList<value_T> >(master_table, master_table+item_count, myTypeName + " SingleLinkList copy entries");
    test_copy< value_T, PooledSingleLinkList<value_T> >(master_table, master_table+item_count, myTypeName + " PooledSingleLinkList copy entries");
    test_copy< value_T, DoubleLinkList<value_T> >(master_table, master_table+item_count, myTypeName + " DoubleLinkList copy entries");
    test_copy< value_T, PooledDoubleLinkList<value_T> >(master_table, master_table+item_count, myTypeName + " PooledDoubleLinkList copy entries");
    
    if (do_summarize)
        summarize("Container copy entries", item_count, iterations, kDontShowGMeans, kDontShowPenalty );
}

/******************************************************************************/

template<typename value_T>
void testAccumulate(const value_T *master_table, size_t item_count, const std::string &myTypeName, size_t iteration_count, bool do_summarize = true ) {

    iterations = iteration_count;

    test_accum_array(master_table, master_table+item_count, myTypeName + " array accumulate");
    test_accum_stdarray(master_table, master_table+item_count, myTypeName + " std::array accumulate");
    test_accum< value_T, std::vector<value_T> >(master_table, master_table+item_count, myTypeName + " std::vector accumulate");
    test_accum< value_T, std::deque<value_T> >(master_table, master_table+item_count, myTypeName + " std::deque accumulate");
    test_accum< value_T, std::forward_list<value_T> >(master_table, master_table+item_count, myTypeName + " std::forward_list accumulate");
    test_accum< value_T, std::list<value_T> >(master_table, master_table+item_count, myTypeName + " std::list accumulate");
    test_accum< value_T, SingleLinkList<value_T> >(master_table, master_table+item_count, myTypeName + " SingleLinkList accumulate");
    test_accum< value_T, PooledSingleLinkList<value_T> >(master_table, master_table+item_count, myTypeName + " PooledSingleLinkList accumulate");
    test_accum_unordered< value_T, PooledSingleLinkList<value_T> >(master_table, master_table+item_count, myTypeName + " PooledSingleLinkList unordered accumulate");
    test_accum< value_T, DoubleLinkList<value_T> >(master_table, master_table+item_count, myTypeName + " DoubleLinkList accumulate");
    test_accum< value_T, PooledDoubleLinkList<value_T> >(master_table, master_table+item_count, myTypeName + " PooledDoubleLinkList accumulate");
    test_accum_unordered< value_T, PooledDoubleLinkList<value_T> >(master_table, master_table+item_count, myTypeName + " PooledDoubleLinkList unordered accumulate");
    test_accum_set< value_T, std::set<value_T> >(master_table, master_table+item_count, myTypeName + " std::set accumulate");
    test_accum_set< value_T, std::multiset<value_T> >(master_table, master_table+item_count, "std::multiset accumulate");
    test_accum_map< value_T, std::map<value_T,value_T> >(master_table, master_table+item_count, myTypeName + "," + myTypeName + " std::map accumulate");
    test_accum_multimap< value_T, std::multimap<value_T,value_T> >(master_table, master_table+item_count, myTypeName + "," + myTypeName + " std::multimap accumulate");
    test_accum_set< value_T, std::unordered_set<value_T> >(master_table, master_table+item_count, myTypeName + " std::unordered_set accumulate");
    test_accum_set< value_T, std::unordered_multiset<value_T> >(master_table, master_table+item_count, myTypeName + " std::unordered_multiset accumulate");
    test_accum_map< value_T, std::unordered_map<value_T,value_T> >(master_table, master_table+item_count, myTypeName + " std::unordered_map accumulate");
    test_accum_multimap< value_T, std::unordered_multimap<value_T,value_T> >(master_table, master_table+item_count, myTypeName + " std::unordered_multimap accumulate");
    test_accum_simplehashmap< value_T, HashMap<value_T,value_T> >(master_table, master_table+item_count, myTypeName + " HashMap accumulate");
    test_accum_simplehashmap< value_T, PooledHashMap<value_T,value_T> >(master_table, master_table+item_count, myTypeName + " PooledHashMap accumulate");
    test_accum_simplehashmap_unordered< value_T, PooledHashMap<value_T,value_T> >(master_table, master_table+item_count, myTypeName + " PooledHashMap unordered accumulate");

    if (do_summarize)
        summarize("Container accumulate", item_count, iterations, kDontShowGMeans, kDontShowPenalty );
}

/******************************************************************************/

template<typename value_T>
void testAccumulateReverse(const value_T *master_table, size_t item_count, const std::string &myTypeName, size_t iteration_count, bool do_summarize = true ) {

    iterations = iteration_count;

    test_accum_array_reverse(master_table, master_table+item_count, myTypeName + " array accumulate reverse");
    test_accum_stdarray_reverse(master_table, master_table+item_count, myTypeName + " std::array accumulate reverse");
    test_accum_reverse< value_T, std::vector<value_T> >(master_table, master_table+item_count, myTypeName + " std::vector accumulate reverse");
    test_accum_reverse< value_T, std::deque<value_T> >(master_table, master_table+item_count, myTypeName + " std::deque accumulate reverse");
    test_accum_reverse< value_T, std::list<value_T> >(master_table, master_table+item_count, myTypeName + " std::list accumulate reverse");
    test_accum_reverse< value_T, DoubleLinkList<value_T> >(master_table, master_table+item_count, myTypeName + " DoubleLinkList accumulate reverse");
    test_accum_reverse< value_T, PooledDoubleLinkList<value_T> >(master_table, master_table+item_count, myTypeName + " PooledDoubleLinkList accumulate reverse");
    test_accum_set_reverse< value_T, std::set<value_T> >(master_table, master_table+item_count, myTypeName + " std::set accumulate reverse");        // implemented, but SLOW
    test_accum_set_reverse< value_T, std::multiset<value_T> >(master_table, master_table+item_count, myTypeName + " std::multiset accumulate reverse");        // implemented, but SLOW
    test_accum_map_reverse< value_T, std::map<value_T,value_T> >(master_table, master_table+item_count, myTypeName + "," + myTypeName + " std::map accumulate reverse");        // implemented, but SLOW
    test_accum_multimap_reverse< value_T, std::multimap<value_T,value_T> >(master_table, master_table+item_count, myTypeName + "," + myTypeName + " std::multimap accumulate reverse");        // implemented, but SLOW
    // unordered containers don't care about iterator direction, and only provide forward iterators
    
    if (do_summarize)
        summarize("Container accumulate reverse", item_count, iterations, kDontShowGMeans, kDontShowPenalty );
}

/******************************************************************************/

template<typename value_T>
void testDuplicate(const value_T *master_table, size_t item_count, const std::string &myTypeName, size_t iteration_count, bool do_summarize = true ) {

    iterations = iteration_count;

    test_duplicate_array(master_table, master_table+item_count, myTypeName + " array duplicate");
    test_duplicate1< value_T, std::vector<value_T> >(master_table, master_table+item_count, myTypeName + " std::vector duplicate");
    test_duplicate1< value_T, std::deque<value_T> >(master_table, master_table+item_count, myTypeName + " std::deque duplicate");
    test_duplicate1< value_T, std::forward_list<value_T> >(master_table, master_table+item_count, myTypeName + " std::forward_list duplicate");
    test_duplicate1< value_T, std::list<value_T> >(master_table, master_table+item_count, myTypeName + " std::list duplicate");
    test_duplicate2< value_T, SingleLinkList<value_T> >(master_table, master_table+item_count, myTypeName + " SingleLinkList duplicate");
    test_duplicate2< value_T, PooledSingleLinkList<value_T> >(master_table, master_table+item_count, myTypeName + " PooledSingleLinkList duplicate");
    test_duplicate2< value_T, DoubleLinkList<value_T> >(master_table, master_table+item_count, myTypeName + " DoubleLinkList duplicate");
    test_duplicate2< value_T, PooledDoubleLinkList<value_T> >(master_table, master_table+item_count, myTypeName + " PooledDoubleLinkList duplicate");
    test_duplicate_set< value_T, std::set<value_T> >(master_table, master_table+item_count, myTypeName + " std::set duplicate");
    test_duplicate_set< value_T, std::multiset<value_T> >(master_table, master_table+item_count, myTypeName + " std::multiset duplicate");
    test_duplicate_map< value_T, std::map<value_T,value_T> >(master_table, master_table+item_count, myTypeName + "," + myTypeName + " std::map duplicate");
    test_duplicate_multimap< value_T, std::multimap<value_T,value_T> >(master_table, master_table+item_count, myTypeName + "," + myTypeName + " std::multimap duplicate");
    test_duplicate_set< value_T, std::unordered_set<value_T> >(master_table, master_table+item_count, myTypeName + " std::unordered_set duplicate");
    test_duplicate_set< value_T, std::unordered_multiset<value_T> >(master_table, master_table+item_count, myTypeName + " std::unordered_multiset duplicate");
    test_duplicate_map< value_T, std::unordered_map<value_T,value_T> >(master_table, master_table+item_count, myTypeName + " std::unordered_map duplicate");
    test_duplicate_multimap< value_T, std::unordered_multimap<value_T,value_T> >(master_table, master_table+item_count, myTypeName + " std::unordered_multimap duplicate");
    test_duplicate_map< value_T, HashMap<value_T,value_T> >(master_table, master_table+item_count, myTypeName + " HashMap duplicate");
    test_duplicate_map< value_T, PooledHashMap<value_T,value_T> >(master_table, master_table+item_count, myTypeName + " PooledHashMap duplicate");

    if (do_summarize)
        summarize("Container duplicate", item_count, iterations, kDontShowGMeans, kDontShowPenalty );
}

/******************************************************************************/

template<typename value_T>
void testPushInsert(const value_T *master_table, size_t item_count, const std::string &myTypeName, size_t iteration_count, bool do_summarize = true ) {

    iterations = iteration_count;
    
    test_pushback< value_T, std::vector<value_T>, true >(master_table, master_table+item_count, myTypeName + " std::vector push_back");
    test_pushback< value_T, std::deque<value_T>, true >(master_table, master_table+item_count, myTypeName + " std::deque push_back");
    test_pushback< value_T, std::list<value_T>, true >(master_table, master_table+item_count, myTypeName + " std::list push_back");
    test_pushback< value_T, SingleLinkList<value_T>, true >(master_table, master_table+item_count, myTypeName + " SingleLinkList push_back");
    test_pushback< value_T, PooledSingleLinkList<value_T>, true >(master_table, master_table+item_count, myTypeName + " PooledSingleLinkList push_back");
    test_pushback< value_T, DoubleLinkList<value_T>, true >(master_table, master_table+item_count, myTypeName + " DoubleLinkList push_back");
    test_pushback< value_T, PooledDoubleLinkList<value_T>, true >(master_table, master_table+item_count, myTypeName + " PooledDoubleLinkList push_back");
    
    if (do_summarize)
        summarize("Container push_back", item_count, iterations, kDontShowGMeans, kDontShowPenalty );


    test_pushfront< value_T, std::deque<value_T>, true >(master_table, master_table+item_count, myTypeName + " std::deque push_front");
    test_pushfront< value_T, std::forward_list<value_T>, true >(master_table, master_table+item_count, myTypeName + " std::forward_list push_front");
    test_pushfront< value_T, std::list<value_T>, true >(master_table, master_table+item_count, myTypeName + " std::list push_front");
    test_pushfront< value_T, SingleLinkList<value_T>, true >(master_table, master_table+item_count, myTypeName + " SingleLinkList push_front");
    test_pushfront< value_T, PooledSingleLinkList<value_T>, true >(master_table, master_table+item_count, myTypeName + " PooledSingleLinkList push_front");
    test_pushfront< value_T, DoubleLinkList<value_T>, true >(master_table, master_table+item_count, myTypeName + " DoubleLinkList push_front");
    test_pushfront< value_T, PooledDoubleLinkList<value_T>, true >(master_table, master_table+item_count, myTypeName + " PooledDoubleLinkList push_front");
    
    if (do_summarize)
        summarize("Container push_front", item_count, iterations, kDontShowGMeans, kDontShowPenalty );
}

/******************************************************************************/

template<typename value_T>
void testInsert_common(value_T *master_table, size_t item_count, const std::string &myTypeName, size_t iteration_count,  const std::string &order) {

    iterations = iteration_count;

    test_insert_set1< value_T, std::set<value_T>, true >(master_table, master_table+item_count, myTypeName + " std::set " + order + " insert");
    test_insert_set1< value_T, std::multiset<value_T>, true >(master_table, master_table+item_count, myTypeName + " std::multiset " + order + " insert");
    test_insert_map< value_T, std::map<value_T, value_T>, true >(master_table, master_table+item_count, myTypeName + "," + myTypeName + " std::map " + order + " insert");
    test_insert_multimap< value_T, std::multimap<value_T, value_T>, true >(master_table, master_table+item_count, myTypeName + "," + myTypeName + " std::multimap " + order + " insert");
    test_insert_set1< value_T, std::unordered_set<value_T>, true >(master_table, master_table+item_count, myTypeName + " std::unordered_set " + order + " insert");
    test_insert_set1< value_T, std::unordered_multiset<value_T>, true >(master_table, master_table+item_count, myTypeName + " std::unordered_multiset " + order + " insert");
    test_insert_map< value_T, std::unordered_map<value_T, value_T>, true >(master_table, master_table+item_count, myTypeName + "," + myTypeName + " std::unordered_map " + order + " insert");
    test_insert_multimap< value_T, std::unordered_multimap<value_T, value_T>, true >(master_table, master_table+item_count, myTypeName + "," + myTypeName + " std::unordered_multimap " + order + " insert");
    test_insert_map< value_T, HashMap<value_T,value_T>, true >(master_table, master_table+item_count, myTypeName + "," + myTypeName + " HashMap " + order + " insert");
    test_insert_map< value_T, PooledHashMap<value_T,value_T>, true >(master_table, master_table+item_count, myTypeName + "," + myTypeName + " PooledHashMap " + order + " insert");
}

/******************************************************************************/

template<typename value_T>
void testInsert_inorder(value_T *master_table, size_t item_count, const std::string &myTypeName, size_t iteration_count, bool do_summarize = true ) {

    testInsert_common<value_T>(master_table,item_count,myTypeName,iteration_count, "in-order");
    
    if (do_summarize)
        summarize("Associative Container in-order insert", item_count, iterations, kDontShowGMeans, kDontShowPenalty );
}

/******************************************************************************/

template<typename value_T>
void testInsert_reverseorder(value_T *master_table, size_t item_count, const std::string &myTypeName, size_t iteration_count, bool do_summarize = true ) {

    // reverse the master list order
    std::reverse( master_table, master_table+item_count );

    testInsert_common<value_T>(master_table,item_count,myTypeName,iteration_count, "reverse order");

    if (do_summarize)
        summarize("Associative Container reverse order insert", item_count, iterations, kDontShowGMeans, kDontShowPenalty );
}

/******************************************************************************/

template<typename value_T>
void testInsert_randomorder(value_T *master_table, size_t item_count, const std::string &myTypeName, size_t iteration_count, bool do_summarize = true ) {

    iterations = iteration_count;
    
    // shuffle the master list to get a random list
    random_shuffle( master_table, master_table+item_count );

    testInsert_common<value_T>(master_table,item_count,myTypeName,iteration_count, "random order");

    if (do_summarize)
        summarize("Associative Container random order insert", item_count, iterations, kDontShowGMeans, kDontShowPenalty );
}

/******************************************************************************/

template<typename value_T>
void testInsert(value_T *master_table, size_t item_count, const std::string &myTypeName, size_t iteration_count, bool do_summarize = true ) {

    testInsert_inorder<value_T>(master_table,item_count,myTypeName,iteration_count,do_summarize);
    testInsert_reverseorder<value_T>(master_table,item_count,myTypeName,iteration_count,do_summarize);
    testInsert_randomorder<value_T>(master_table,item_count,myTypeName,iteration_count,do_summarize);
}

/******************************************************************************/

template<typename value_T>
void testDelete(value_T *master_table, size_t item_count, const std::string &myTypeName, size_t iteration_count, bool do_summarize = true ) {

    iterations = iteration_count;
    
    // make unique master list, in order
    for( int i = 0; i < item_count; ++i )
        master_table[i] = static_cast<value_T>( i * 3 );

    // shuffle the master list to get a random list
    random_shuffle( master_table, master_table+item_count );
    
    test_delete_pushback< value_T, std::vector<value_T> >(master_table, master_table+item_count, myTypeName + " std::vector delete");
    test_delete_pushback< value_T, std::deque<value_T> >(master_table, master_table+item_count, myTypeName + " std::deque delete");
    test_delete_forward< value_T, std::forward_list<value_T> >(master_table, master_table+item_count, myTypeName + " std::forward_list delete");
    test_delete_pushback< value_T, std::list<value_T> >(master_table, master_table+item_count, myTypeName + " std::list delete");
    test_delete_pushback< value_T, SingleLinkList<value_T> >(master_table, master_table+item_count, myTypeName + " SingleLinkList delete");
    test_delete_pushback< value_T, PooledSingleLinkList<value_T> >(master_table, master_table+item_count, myTypeName + " PooledSingleLinkList delete");
    test_delete_pushback< value_T, DoubleLinkList<value_T> >(master_table, master_table+item_count, myTypeName + " DoubleLinkList delete");
    test_delete_pushback< value_T, PooledDoubleLinkList<value_T> >(master_table, master_table+item_count, myTypeName + " PooledDoubleLinkList delete");
    test_delete_set1< value_T, std::set<value_T> >(master_table, master_table+item_count, myTypeName + " std::set delete");
    test_delete_set1< value_T, std::multiset<value_T> >(master_table, master_table+item_count, myTypeName + " std::multiset delete");
    test_delete_map< value_T, std::map<value_T, value_T> >(master_table, master_table+item_count, myTypeName + "," + myTypeName + " std::map delete");
    test_delete_multimap< value_T, std::multimap<value_T, value_T> >(master_table, master_table+item_count, myTypeName + "," + myTypeName + " std::multimap delete");
    test_delete_set1< value_T, std::unordered_set<value_T> >(master_table, master_table+item_count, myTypeName + " std::unordered_set delete");
    test_delete_set1< value_T, std::unordered_multiset<value_T> >(master_table, master_table+item_count, myTypeName + " std::unordered_multiset delete");
    test_delete_map< value_T, std::unordered_map<value_T, value_T> >(master_table, master_table+item_count, myTypeName + "," + myTypeName + " std::unordered_map delete");
    test_delete_multimap< value_T, std::unordered_multimap<value_T, value_T> >(master_table, master_table+item_count, myTypeName + "," + myTypeName + " std::unordered_multimap delete");
    test_delete_map< value_T, HashMap<value_T,value_T> >(master_table, master_table+item_count, myTypeName + "," + myTypeName + " HashMap delete");
    test_delete_map< value_T, PooledHashMap<value_T,value_T> >(master_table, master_table+item_count, myTypeName + "," + myTypeName + " PooledHashMap delete");

    if (do_summarize)
        summarize("Container delete", item_count, iterations, kDontShowGMeans, kDontShowPenalty );
}

/******************************************************************************/

template<typename value_T>
void testEraseAll(value_T *master_table, size_t item_count, const std::string &myTypeName, size_t iteration_count, bool do_summarize = true ) {

    iterations = iteration_count;
    
    // make unique master list, in order
    for( int i = 0; i < item_count; ++i )
        master_table[i] = static_cast<value_T>( i * 3 );

    // shuffle the master list to get a random list
    random_shuffle( master_table, master_table+item_count );

    test_eraseall_pushback< value_T, std::vector<value_T> >(master_table, master_table+item_count, myTypeName + " std::vector erase all entries");
    test_eraseall_pushback< value_T, std::deque<value_T> >(master_table, master_table+item_count, myTypeName + " std::deque erase all entries");
    //test_eraseall_forward< value_T, std::forward_list<value_T> >(master_table, master_table+item_count, myTypeName + " std::forward_list erase all entries");    // no erase implementation
    test_eraseall_pushback< value_T, std::list<value_T> >(master_table, master_table+item_count, myTypeName + " std::list erase all entries");
    test_eraseall_pushback< value_T, SingleLinkList<value_T> >(master_table, master_table+item_count, myTypeName + " SingleLinkList erase all entries");
    test_eraseall_pushback< value_T, PooledSingleLinkList<value_T> >(master_table, master_table+item_count, myTypeName + " PooledSingleLinkList erase all entries");
    test_eraseall_pushback< value_T, DoubleLinkList<value_T> >(master_table, master_table+item_count, myTypeName + " DoubleLinkList erase all entries");
    test_eraseall_pushback< value_T, PooledDoubleLinkList<value_T> >(master_table, master_table+item_count, myTypeName + " PooledDoubleLinkList erase all entries");
    test_eraseall_set1< value_T, std::set<value_T> >(master_table, master_table+item_count, myTypeName + " std::set erase all entries");
    test_eraseall_set1< value_T, std::multiset<value_T> >(master_table, master_table+item_count, myTypeName + " std::multiset erase all entries");
    test_eraseall_map< value_T, std::map<value_T, value_T> >(master_table, master_table+item_count, myTypeName + "," + myTypeName + " std::map erase all entries");
    test_eraseall_multimap< value_T, std::multimap<value_T, value_T> >(master_table, master_table+item_count, myTypeName + "," + myTypeName + " std::multimap erase all entries");
    test_eraseall_set1< value_T, std::unordered_set<value_T> >(master_table, master_table+item_count, myTypeName + " std::unordered_set erase all entries");
    test_eraseall_set1< value_T, std::unordered_multiset<value_T> >(master_table, master_table+item_count, myTypeName + " std::unordered_multiset erase all entries");
    test_eraseall_map< value_T, std::unordered_map<value_T, value_T> >(master_table, master_table+item_count, myTypeName + "," + myTypeName + " std::unordered_map erase all entries");
    test_eraseall_multimap< value_T, std::unordered_multimap<value_T, value_T> >(master_table, master_table+item_count, myTypeName + "," + myTypeName + " std::unordered_multimap erase all entries");
    test_eraseall_map< value_T, HashMap<value_T,value_T> >(master_table, master_table+item_count, myTypeName + "," + myTypeName + " HashMap erase all entries");
    test_eraseall_map< value_T, PooledHashMap<value_T,value_T> >(master_table, master_table+item_count, myTypeName + "," + myTypeName + " PooledHashMap erase all entries");

    if (do_summarize)
        summarize("Container erase all entries", item_count, iterations, kDontShowGMeans, kDontShowPenalty );
}

/******************************************************************************/

template<typename value_T>
void testClearAll(value_T *master_table, size_t item_count, const std::string &myTypeName, size_t iteration_count, bool do_summarize = true ) {

    iterations = iteration_count;
    
    // make unique master list, in order
    for( int i = 0; i < item_count; ++i )
        master_table[i] = static_cast<value_T>( i * 3 );

    // shuffle the master list to get a random list
    random_shuffle( master_table, master_table+item_count );

    test_clearall_pushback< value_T, std::vector<value_T> >(master_table, master_table+item_count, myTypeName + " std::vector clear all entries");
    test_clearall_pushback< value_T, std::deque<value_T> >(master_table, master_table+item_count, myTypeName + " std::deque clear all entries");
    test_clearall_forward< value_T, std::forward_list<value_T> >(master_table, master_table+item_count, myTypeName + " std::forward_list clear all entries");
    test_clearall_pushback< value_T, std::list<value_T> >(master_table, master_table+item_count, myTypeName + " std::list clear all entries");
    test_clearall_pushback< value_T, SingleLinkList<value_T> >(master_table, master_table+item_count, myTypeName + " SingleLinkList clear all entries");
    test_clearall_pushback< value_T, PooledSingleLinkList<value_T> >(master_table, master_table+item_count, myTypeName + " PooledSingleLinkList clear all entries");
    test_clearall_pushback< value_T, DoubleLinkList<value_T> >(master_table, master_table+item_count, myTypeName + " DoubleLinkList clear all entries");
    test_clearall_pushback< value_T, PooledDoubleLinkList<value_T> >(master_table, master_table+item_count, myTypeName + " PooledDoubleLinkList clear all entries");
    test_clearall_set1< value_T, std::set<value_T> >(master_table, master_table+item_count, myTypeName + " std::set clear all entries");
    test_clearall_set1< value_T, std::multiset<value_T> >(master_table, master_table+item_count, myTypeName + " std::multiset clear all entries");
    test_clearall_map< value_T, std::map<value_T, value_T> >(master_table, master_table+item_count, myTypeName + "," + myTypeName + " std::map clear all entries");
    test_clearall_multimap< value_T, std::multimap<value_T, value_T> >(master_table, master_table+item_count, myTypeName + "," + myTypeName + " std::multimap clear all entries");
    test_clearall_set1< value_T, std::unordered_set<value_T> >(master_table, master_table+item_count, myTypeName + " std::unordered_set clear all entries");
    test_clearall_set1< value_T, std::unordered_multiset<value_T> >(master_table, master_table+item_count, myTypeName + " std::unordered_multiset clear all entries");
    test_clearall_map< value_T, std::unordered_map<value_T, value_T> >(master_table, master_table+item_count, myTypeName + "," + myTypeName + " std::unordered_map clear all entries");
    test_clearall_multimap< value_T, std::unordered_multimap<value_T, value_T> >(master_table, master_table+item_count, myTypeName + "," + myTypeName + " std::unordered_multimap clear all entries");
    test_clearall_map< value_T, HashMap<value_T,value_T> >(master_table, master_table+item_count, myTypeName + "," + myTypeName + " HashMap clear all entries");
    test_clearall_map< value_T, PooledHashMap<value_T,value_T> >(master_table, master_table+item_count, myTypeName + "," + myTypeName + " PooledHashMap clear all entries");

    if (do_summarize)
        summarize("Container clear all entries", item_count, iterations, kDontShowGMeans, kDontShowPenalty );
}

/******************************************************************************/

template<typename value_T>
void testPop(value_T *master_table, size_t item_count, const std::string &myTypeName, size_t iteration_count, bool do_summarize = true ) {

    iterations = iteration_count;
    
    // make unique master list, in order
    for( int i = 0; i < item_count; ++i )
        master_table[i] = static_cast<value_T>( i * 3 );

    // shuffle the master list to get a random list
    random_shuffle( master_table, master_table+item_count );

    test_popfront< value_T, std::deque<value_T> >(master_table, master_table+item_count, myTypeName + " std::deque pop_front");
    test_popfront_forward< value_T, std::forward_list<value_T> >(master_table, master_table+item_count, myTypeName + " std::forward_list pop_front");
    test_popfront< value_T, std::list<value_T> >(master_table, master_table+item_count, myTypeName + " std::list pop_front");
    test_popfront< value_T, SingleLinkList<value_T> >(master_table, master_table+item_count, myTypeName + " SingleLinkList pop_front");
    test_popfront< value_T, PooledSingleLinkList<value_T> >(master_table, master_table+item_count, myTypeName + " PooledSingleLinkList pop_front");
    test_popfront< value_T, DoubleLinkList<value_T> >(master_table, master_table+item_count, myTypeName + " DoubleLinkList pop_front");
    test_popfront< value_T, PooledDoubleLinkList<value_T> >(master_table, master_table+item_count, myTypeName + " PooledDoubleLinkList pop_front");

    if (do_summarize)
        summarize("Container pop_front", item_count, iterations, kDontShowGMeans, kDontShowPenalty );


    test_popback< value_T, std::vector<value_T> >(master_table, master_table+item_count, myTypeName + " std::vector pop_back");
    test_popback< value_T, std::deque<value_T> >(master_table, master_table+item_count, myTypeName + " std::deque pop_back");
    test_popback< value_T, std::list<value_T> >(master_table, master_table+item_count, myTypeName + " std::list pop_back");
//    test_popback< value_T, SingleLinkList<value_T> >(master_table, master_table+item_count, myTypeName + " SingleLinkList pop_back");    // insanely slow O(N^2), only implemented for debugging
//    test_popback< value_T, PooledSingleLinkList<value_T> >(master_table, master_table+item_count, myTypeName + " PooledSingleLinkList pop_back");    // insanely slow O(N^2), only implemented for debugging
    test_popback< value_T, DoubleLinkList<value_T> >(master_table, master_table+item_count, myTypeName + " DoubleLinkList pop_back");
    test_popback< value_T, PooledDoubleLinkList<value_T> >(master_table, master_table+item_count, myTypeName + " PooledDoubleLinkList pop_back");

    if (do_summarize)
        summarize("Container pop_back", item_count, iterations, kDontShowGMeans, kDontShowPenalty );
}

/******************************************************************************/

template<typename value_T>
void testFind_common(value_T *master_table, value_T *lookup_table, size_t item_count, const std::string &myTypeName, size_t iteration_count, const std::string &order ) {

    iterations = iteration_count;

    test_find_set1< value_T, std::set<value_T> >(master_table, master_table+item_count, lookup_table, lookup_table+item_count, myTypeName + " std::set " + order + " find");
    test_find_set1< value_T, std::multiset<value_T> >(master_table, master_table+item_count, lookup_table, lookup_table+item_count, myTypeName + " std::multiset " + order + " find");
    test_find_map< value_T, std::map<value_T, value_T> >(master_table, master_table+item_count, lookup_table, lookup_table+item_count, myTypeName + "," + myTypeName + " std::map " + order + " find");
    test_find_multimap< value_T, std::multimap<value_T, value_T> >(master_table, master_table+item_count, lookup_table, lookup_table+item_count, myTypeName + "," + myTypeName + " std::multimap " + order + " find");
    test_find_set1< value_T, std::unordered_set<value_T> >(master_table, master_table+item_count, lookup_table, lookup_table+item_count, myTypeName + " std::unordered_set " + order + " find");
    test_find_set1< value_T, std::unordered_multiset<value_T> >(master_table, master_table+item_count, lookup_table, lookup_table+item_count, myTypeName + " std::unordered_multiset " + order + " find");
    test_find_map< value_T, std::unordered_map<value_T, value_T> >(master_table, master_table+item_count, lookup_table, lookup_table+item_count, myTypeName + "," + myTypeName + " std::unordered_map " + order + " find");
    test_find_multimap< value_T, std::unordered_multimap<value_T, value_T> >(master_table, master_table+item_count, lookup_table, lookup_table+item_count, myTypeName + "," + myTypeName + " std::unordered_multimap " + order + " find");
    test_find_simplehashmap< value_T, HashMap<value_T,value_T> >(master_table, master_table+item_count, lookup_table, lookup_table+item_count, myTypeName + "," + myTypeName + " HashMap " + order + " find");
    test_find_simplehashmap< value_T, PooledHashMap<value_T,value_T> >(master_table, master_table+item_count, lookup_table, lookup_table+item_count, myTypeName + "," + myTypeName + " PooledHashMap " + order + " find");
    test_find_pushback_sorted< value_T, std::vector<value_T> >(master_table, master_table+item_count, lookup_table, lookup_table+item_count, myTypeName + " sorted std::vector " + order + " find");
    test_find_pushback_sorted< value_T, std::deque<value_T> >(master_table, master_table+item_count, lookup_table, lookup_table+item_count, myTypeName + " sorted std::deque " + order + " find");

#if TEST_SLOW_FINDS
    test_find_pushback< value_T, std::vector<value_T> >(master_table, master_table+item_count, lookup_table, lookup_table+item_count, myTypeName + " std::vector " + order + " find");
    test_find_pushback< value_T, std::deque<value_T> >(master_table, master_table+item_count, lookup_table, lookup_table+item_count, myTypeName + " std::deque " + order + " find");
    test_find_pushback_forward< value_T, std::forward_list<value_T> >(master_table, master_table+item_count, lookup_table, lookup_table+item_count, myTypeName + " std::forward_list " + order + " find");
    test_find_pushback< value_T, std::list<value_T> >(master_table, master_table+item_count, lookup_table, lookup_table+item_count, myTypeName + " std::list " + order + " find");
    test_find_pushback< value_T, SingleLinkList<value_T> >(master_table, master_table+item_count, lookup_table, lookup_table+item_count, myTypeName + " SingleLinkList " + order + " find");
    test_find_pushback< value_T, PooledSingleLinkList<value_T> >(master_table, master_table+item_count, lookup_table, lookup_table+item_count, myTypeName + " PooledSingleLinkList " + order + " find");
    test_find_pushback< value_T, DoubleLinkList<value_T> >(master_table, master_table+item_count, lookup_table, lookup_table+item_count, myTypeName + " DoubleLinkList " + order + " find");
    test_find_pushback< value_T, PooledDoubleLinkList<value_T> >(master_table, master_table+item_count, lookup_table, lookup_table+item_count, myTypeName + " PooledDoubleLinkList " + order + " find");
#endif
}

/******************************************************************************/

template<typename value_T>
void testFind_inorder(value_T *master_table, value_T *lookup_table, size_t item_count, const std::string &myTypeName, size_t iteration_count, bool do_summarize = true ) {

    iterations = iteration_count;
    
    // make unique master list, in order
    for( int i = 0; i < item_count; ++i )
        master_table[i] = static_cast<value_T>( i * 3 );

    // copy the master list to get an in-order lookup list
    std::copy( master_table, master_table+item_count, lookup_table );
    
    testFind_common<value_T>(master_table,lookup_table,item_count,myTypeName,iteration_count, "in-order");

    if (do_summarize)
        summarize("Container find in-order", item_count, iterations, kDontShowGMeans, kDontShowPenalty );
}

/******************************************************************************/

template<typename value_T>
void testFind_reverseorder(value_T *master_table, value_T *lookup_table, size_t item_count, const std::string &myTypeName, size_t iteration_count, bool do_summarize = true ) {

    iterations = iteration_count;
    
    // make unique master list, in order
    for( int i = 0; i < item_count; ++i )
        master_table[i] = static_cast<value_T>( i * 3 );

    // copy the master list to get an in-order lookup list
    std::copy( master_table, master_table+item_count, lookup_table );

    // reverse the lookup list order
    std::reverse( lookup_table, lookup_table+item_count );
    
    testFind_common<value_T>(master_table,lookup_table,item_count,myTypeName,iteration_count, "reverse order");
    
    if (do_summarize)
        summarize("Container find reverse order", item_count, iterations, kDontShowGMeans, kDontShowPenalty );
}

/******************************************************************************/

template<typename value_T>
void testFind_randomorder(value_T *master_table, value_T *lookup_table, size_t item_count, const std::string &myTypeName, size_t iteration_count, bool do_summarize = true ) {

    iterations = iteration_count;
    
    // make unique master list, in order
    for( int i = 0; i < item_count; ++i )
        master_table[i] = static_cast<value_T>( i * 3 );

    // copy the master list to get an in-order lookup list
    std::copy( master_table, master_table+item_count, lookup_table );
    
    // shuffle the lookup list to get a random list
    random_shuffle( lookup_table, lookup_table+item_count );
    
    testFind_common<value_T>(master_table,lookup_table,item_count,myTypeName,iteration_count, "random order");

    if (do_summarize)
        summarize("Container find random order", item_count, iterations, kDontShowGMeans, kDontShowPenalty );
}

/******************************************************************************/

template<typename value_T>
void testFind(value_T *master_table, value_T *lookup_table, size_t item_count, const std::string &myTypeName, size_t iteration_count, bool do_summarize = true ) {

    testFind_inorder<value_T>(master_table, lookup_table, item_count, myTypeName, iteration_count, do_summarize );
    testFind_reverseorder<value_T>(master_table, lookup_table, item_count, myTypeName, iteration_count, do_summarize );
    testFind_randomorder<value_T>(master_table, lookup_table, item_count, myTypeName, iteration_count, do_summarize );
}

/******************************************************************************/

template<typename value_T>
void testErase_common(const value_T *master_table, value_T *lookup_table, size_t item_count, const std::string &myTypeName, size_t iteration_count, const std::string &order) {

    iterations = iteration_count;

    test_erase_set1< value_T, std::set<value_T> >(master_table, master_table+item_count, lookup_table, lookup_table+item_count, myTypeName + " std::set " + order + " erase");
    test_erase_set1< value_T, std::multiset<value_T> >(master_table, master_table+item_count, lookup_table, lookup_table+item_count, myTypeName + " std::multiset " + order + " erase");
    test_erase_map< value_T, std::map<value_T, value_T> >(master_table, master_table+item_count, lookup_table, lookup_table+item_count, myTypeName + "," + myTypeName + " std::map " + order + " erase");
    test_erase_multimap< value_T, std::multimap<value_T, value_T> >(master_table, master_table+item_count, lookup_table, lookup_table+item_count, myTypeName + "," + myTypeName + " std::multimap " + order + " erase");
    test_erase_set1< value_T, std::unordered_set<value_T> >(master_table, master_table+item_count, lookup_table, lookup_table+item_count, myTypeName + " std::unordered_set " + order + " erase");
    test_erase_set1< value_T, std::unordered_multiset<value_T> >(master_table, master_table+item_count, lookup_table, lookup_table+item_count, myTypeName + " std::unordered_multiset " + order + " erase");
    test_erase_map< value_T, std::unordered_map<value_T, value_T> >(master_table, master_table+item_count, lookup_table, lookup_table+item_count, myTypeName + "," + myTypeName + " std::unordered_map " + order + " erase");
    test_erase_multimap< value_T, std::unordered_multimap<value_T, value_T> >(master_table, master_table+item_count, lookup_table, lookup_table+item_count, myTypeName + "," + myTypeName + " std::unordered_multimap " + order + " erase");
    test_erase_map< value_T, HashMap<value_T,value_T> >(master_table, master_table+item_count, lookup_table, lookup_table+item_count, myTypeName + "," + myTypeName + " HashMap " + order + " erase");
    test_erase_map< value_T, PooledHashMap<value_T,value_T> >(master_table, master_table+item_count, lookup_table, lookup_table+item_count, myTypeName + "," + myTypeName + " PooledHashMap " + order + " erase");
    test_erase_pushback_sorted< value_T, std::vector<value_T> >(master_table, master_table+item_count, lookup_table, lookup_table+item_count, myTypeName + " sorted std::vector " + order + " erase");
    test_erase_pushback_sorted< value_T, std::deque<value_T> >(master_table, master_table+item_count, lookup_table, lookup_table+item_count, myTypeName + " sorted std::deque " + order + " erase");

#if TEST_SLOW_FINDS
    test_erase_pushback< value_T, std::vector<value_T> >(master_table, master_table+item_count, lookup_table, lookup_table+item_count, myTypeName + " std::vector " + order + " erase");
    test_erase_pushback< value_T, std::deque<value_T> >(master_table, master_table+item_count, lookup_table, lookup_table+item_count, myTypeName + " std::deque " + order + " erase");
    test_erase_pushback_forward< value_T, std::forward_list<value_T> >(master_table, master_table+item_count, lookup_table, lookup_table+item_count, myTypeName + " std::forward_list " + order + " erase");
    test_erase_pushback< value_T, std::list<value_T> >(master_table, master_table+item_count, lookup_table, lookup_table+item_count, myTypeName + " std::list " + order + " erase");
    test_erase_pushback_forward< value_T, SingleLinkList<value_T> >(master_table, master_table+item_count, lookup_table, lookup_table+item_count, myTypeName + " SingleLinkList " + order + " erase");
    test_erase_pushback_forward< value_T, PooledSingleLinkList<value_T> >(master_table, master_table+item_count, lookup_table, lookup_table+item_count, myTypeName + " PooledSingleLinkList " + order + " erase");
    test_erase_pushback< value_T, DoubleLinkList<value_T> >(master_table, master_table+item_count, lookup_table, lookup_table+item_count, myTypeName + " DoubleLinkList " + order + " erase");
    test_erase_pushback< value_T, PooledDoubleLinkList<value_T> >(master_table, master_table+item_count, lookup_table, lookup_table+item_count, myTypeName + " PooledDoubleLinkList " + order + " erase");
#endif

}

/******************************************************************************/

template<typename value_T>
void testErase_inorder(const value_T *master_table, value_T *lookup_table, size_t item_count, const std::string &myTypeName, size_t iteration_count, bool do_summarize = true ) {

    iterations = iteration_count;

    // copy the master list to get an in-order lookup list
    std::copy( master_table, master_table+item_count, lookup_table );
    
    testErase_common<value_T>(master_table,lookup_table,item_count,myTypeName,iteration_count,"in-order");

    if (do_summarize)
        summarize("Container erase in-order", item_count, iterations, kDontShowGMeans, kDontShowPenalty );
}

/******************************************************************************/

template<typename value_T>
void testErase_reverseorder(const value_T *master_table, value_T *lookup_table, size_t item_count, const std::string &myTypeName, size_t iteration_count, bool do_summarize = true ) {

    iterations = iteration_count;

    // copy the master list to get an in-order lookup list
    std::copy( master_table, master_table+item_count, lookup_table );

    // reverse the lookup list order
    std::reverse( lookup_table, lookup_table+item_count );
    
    testErase_common<value_T>(master_table,lookup_table,item_count,myTypeName,iteration_count,"reverse order");
    
    if (do_summarize)
        summarize("Container erase reverse order", item_count, iterations, kDontShowGMeans, kDontShowPenalty );
}

/******************************************************************************/

template<typename value_T>
void testErase_randomorder(const value_T *master_table, value_T *lookup_table, size_t item_count, const std::string &myTypeName, size_t iteration_count, bool do_summarize = true ) {

    iterations = iteration_count;

    // copy the master list to get an in-order lookup list
    std::copy( master_table, master_table+item_count, lookup_table );
    
    // shuffle the lookup list to get a random list
    random_shuffle( lookup_table, lookup_table+item_count );
    
    testErase_common<value_T>(master_table,lookup_table,item_count,myTypeName,iteration_count,"random order");

    if (do_summarize)
        summarize("Container erase random order", item_count, iterations, kDontShowGMeans, kDontShowPenalty );
}

/******************************************************************************/

template<typename value_T>
void testErase(const value_T *master_table, value_T *lookup_table, size_t item_count, const std::string &myTypeName, size_t iteration_count, bool do_summarize = true ) {

    testErase_inorder<value_T>(master_table, lookup_table, item_count, myTypeName, iteration_count, do_summarize );
    testErase_reverseorder<value_T>(master_table, lookup_table, item_count, myTypeName, iteration_count, do_summarize );
    testErase_randomorder<value_T>(master_table, lookup_table, item_count, myTypeName, iteration_count, do_summarize );
}

/******************************************************************************/
/******************************************************************************/

// format is tab separated columns
// TODO - ccox - work in progress, move this to benchmark_results.h
void summarize_spreadhseet( FILE *output, const int size, const int iterations ) {
    int i;
    static bool firstOutput = true;

    if (firstOutput) {
        fprintf(output,"size\t");
        for (i = 0; i < current_test; ++i)
            fprintf(output,"%s\t", results[i].label);
        fprintf(output,"\n");

        firstOutput = false;
    }
    
    
    fprintf(output, "%d\t", size);        // just in case you don't increment by 1

#if 0
    // millions of operations per second
    const double millions = ((double)(size) * iterations)/1.0e6;
    for (i = 0; i < current_test; ++i) {
        double op_time = results[i].time;
        if (op_time > 0.0)
            fprintf(output, "%g\t", millions/results[i].time );
        else
            fprintf(output, "infinity\t");
    }
#elif 0
    // nano seconds per operation
    for (i = 0; i < current_test; ++i) {
        double time_per_op = 1.0e9 * results[i].time/((double)iterations * (double)size);
        if (time_per_op < 0.0)
            time_per_op = 0.0;
        fprintf(output, "%g\t", time_per_op );
    }
#else
    // milli seconds per iteration
    for (i = 0; i < current_test; ++i) {
        double time_per_iter = 1.0e6 * results[i].time/((double)iterations);
        if (time_per_iter < 0.0)
            time_per_iter = 0.0;
        fprintf(output, "%g\t", time_per_iter );
    }
#endif

    fprintf(output,"\n");
    fflush(output);

    // reset the test counter so we can run more tests
    current_test = 0;
}

/******************************************************************************/

// TODO - ccox - work in progress
// WARNING - ccox - this can take a day or more to run
template<typename value_T>
void create_spreadsheet(int argc, char** argv) {

    printf("Creating container timing spreadsheet...\n");
    
    std::string myTypeName( getTypeName<value_T>() );
    
    gLabels.clear();

    size_t graph_maximum = 64*1024*1024;    // 512 Meg of data
    float graph_increment = 1.10;           // 10% increment with each step

    base_iterations = 50000;
    
    // create spreadsheet file
    std::string output_filename = "container_timings_" + myTypeName + ".txt";
    FILE *spreadsheet = fopen(output_filename.c_str(),"w");
    if (!spreadsheet) {
        printf("%s could not create output file %s\n", argv[0], output_filename.c_str());
        return;
    }
    
    try {
    
        value_T *graph_table = new value_T[ graph_maximum ];
        value_T *graph_lookup_table = new value_T[ graph_maximum ];
        
        for( size_t i = 0; i < graph_maximum; ++i )
            graph_table[i] = static_cast<value_T>( i * 3 );

        random_shuffle( graph_table, graph_table+graph_maximum );

        clock_t last_status = 0;

        for( size_t current_size = 4; current_size <= graph_maximum; ) {

            // print status every once in a while, just to prove the process isn't hung :-)
            if ((clock() - last_status) > (clock_t)(2*CLOCKS_PER_SEC)) {
                printf("testing %d\n", (int)current_size );
                last_status = clock();
            }

            // seed the random number generator, so we get repeatable results
            scrand( base_iterations + 123 );

            // try to keep the time more or less constant for all tests (short tests need more iterations, etc.)
            iterations = base_iterations / current_size;
            // and impose a minimum iteration count to average out timing noise
            if (iterations < 4)
                iterations = 4;
            
            testPushInsert<value_T>(graph_table,current_size,myTypeName,iterations,false);
            testInsert_randomorder<value_T>(graph_table,current_size,myTypeName,iterations,false);
            testFind_randomorder<value_T>(graph_table,graph_lookup_table,current_size,myTypeName,iterations,false);
            testAccumulate<value_T>(graph_table,current_size,myTypeName,iterations,false);
            testDuplicate<value_T>(graph_table,current_size,myTypeName,iterations,false);
            testDelete<value_T>(graph_table,current_size,myTypeName,iterations,false);
            testEraseAll<value_T>(graph_table,current_size,myTypeName,iterations,false);
            testClearAll<value_T>(graph_table,current_size,myTypeName,iterations,false);
            testErase_randomorder<value_T>(graph_table,graph_lookup_table,current_size,myTypeName,iterations,false);
    
            // format results for spreadsheet
            summarize_spreadhseet( spreadsheet, current_size, iterations );
            
            // calculate our next size
            if (current_size == graph_maximum)
                break;
            
            size_t new_size( ceil(current_size * graph_increment) );
            assert(new_size != current_size);
            current_size = new_size;
            
            if (current_size > graph_maximum)
                current_size = graph_maximum;
        }

        delete[] graph_table;
        delete[] graph_lookup_table;
    }
    catch( const std::exception &ex ) {
        fprintf(stderr,"spreadsheet aborted due to exception: %s\n", ex.what() );
        fprintf(spreadsheet,"spreadsheet aborted due to exception: %s\n", ex.what() );
    }
    catch( ... ) {
        fprintf(stderr,"spreadsheet aborted due to unknown exception\n");
        fprintf(spreadsheet,"spreadsheet aborted due to unknown exception\n");
    }

    fclose(spreadsheet);

}

/******************************************************************************/
/******************************************************************************/

template<typename value_T>
void TestOneType() {

    value_T master_table[ SIZE ];
    value_T lookup_table[ SIZE ];
    
    std::string myTypeName( getTypeName<value_T>() );
    
    gLabels.clear();
    
    // seed the random number generator, so we get repeatable results
    scrand( base_iterations + 123 );
    
    // random values make a mess of set and map tests (because you can get duplicates)
    // make unique master list, in order
    for( int i = 0; i < SIZE; ++i )
        master_table[i] = static_cast<value_T>( i * 3 );

    // shuffle the master list to get a random list
    random_shuffle( master_table, master_table+SIZE );

    testCopyEntries<value_T>(master_table, SIZE, myTypeName, base_iterations );
    testAccumulate<value_T>(master_table, SIZE, myTypeName, base_iterations / 5 );
    testAccumulateReverse<value_T>(master_table, SIZE, myTypeName, base_iterations / 5 );
    testDuplicate<value_T>(master_table, SIZE, myTypeName, base_iterations / 100 );
    testPushInsert<value_T>(master_table, SIZE, myTypeName, base_iterations / 10 );
    
    
    // make unique master list, in increasing order (gets shuffled inside test routine)
    for( int i = 0; i < SIZE; ++i )
        master_table[i] = static_cast<value_T>( i * 3 );
    
    testInsert<value_T>(master_table, SIZE, myTypeName, base_iterations / 200 );
    testDelete<value_T>(master_table, SIZE, myTypeName, base_iterations / 100 );
    testEraseAll<value_T>(master_table, SIZE, myTypeName, base_iterations / 10 );
    testClearAll<value_T>(master_table, SIZE, myTypeName, base_iterations / 10 );
    testPop<value_T>(master_table, SIZE, myTypeName, base_iterations / 100 );
    testFind<value_T>(master_table, lookup_table, SIZE, myTypeName, base_iterations / 30 );
    testErase<value_T>(master_table, lookup_table, SIZE, myTypeName, base_iterations / 30 );

}

/******************************************************************************/
/******************************************************************************/

int main(int argc, char** argv) {
    int do_spreadsheet = 0;

    // output command for documentation:
    int i;
    for (i = 0; i < argc; ++i)
        printf("%s ", argv[i] );
    printf("\n");

    if (argc > 1) base_iterations = atoi(argv[1]);
    if (argc > 2) do_spreadsheet = (int) atoi(argv[2]);
    
    
    iterations = base_iterations;

    if (do_spreadsheet != 0) {
        create_spreadsheet<double>(argc, argv);
        return 0;
    }


    TestOneType<double>();


#if WORKS_BUT_NOT_NEEDED
    //TestOneType<uint8_t>();     // can have value and key aliasing, affecting results
    //TestOneType<int8_t>();     // can have value and key aliasing, affecting results
    TestOneType<uint16_t>();
    TestOneType<int16_t>();
    TestOneType<uint32_t>();
    TestOneType<int32_t>();
    TestOneType<uint64_t>();
    TestOneType<int64_t>();
    TestOneType<float>();
    TestOneType<long double>();
#endif

    return 0;
}

// the end
/******************************************************************************/
/******************************************************************************/
