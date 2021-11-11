/*
    Copyright 2019 Chris Cox
    Distributed under the MIT License (see accompanying file LICENSE_1_0_0.txt
    or a copy at http://stlab.adobe.com/licenses.html )


Goals: Compare the performance of various binary search implementations,
        and compiler optimizations applied to those implementations and containers.


Assumptions:

    1) Compilers will not mess up algorithms this simple.
 
    2) STL implementations of lower_bound, upper_bound, and binary_search will be optimized correctly.
        (typically binary_search is implented by calling lower_bound - but this is not a good idea)



See https://en.wikipedia.org/wiki/Binary_search_algorithm

TODO: https://lemire.me/blog/2019/09/14/speeding-up-independent-binary-searches-by-interleaving-them/

*/


/******************************************************************************/

#include "benchmark_stdint.hpp"
#include <functional>
#include <cstdlib>
#include <cstdio>
#include <cmath>
#include <cstddef>
#include <cstring>
#include <memory>
#include <forward_list>
#include <list>
#include <deque>
#include <vector>
#include <algorithm>
#include <functional>
#include "benchmark_timer.h"
#include "benchmark_algorithms.h"
#include "benchmark_typenames.h"

using namespace std;

/******************************************************************************/
/******************************************************************************/

// How long do we want to iterate each test, at a minimum?
//      Increasing the time improves precision, but also increases the total benchmark run time.
//      Doubling the minimum time will approximately double the total time.
//      But precision will be affected by the OS and system variability, and there isn't as clean a relationship.

// Currently takes around 3 hours to run full benchmark at default value.
double gMinimumTimeTarget = 0.20;  // in seconds

/******************************************************************************/
/******************************************************************************/

// straightforward version, but slow for non-random access iterators
template <typename ForwardIterator, typename T>
ForwardIterator lower_bound1(ForwardIterator left, ForwardIterator right, const T value)
{
    while (left != right) {
        auto len = std::distance(left,right);
        auto halfway = len / 2;
        
        auto mid = left;
        std::advance(mid, halfway);
        
        if ( *mid < value  )
            left = ++mid;
        else
            right = mid;
    }

    return left;
}

/******************************************************************************/

// don't recompute the distance every time
template <typename ForwardIterator, typename T>
ForwardIterator lower_bound2(ForwardIterator left, ForwardIterator right, const T value)
{
    auto len = std::distance(left,right);
    
    while (len != 0)  {
        auto halfway = len / 2;
        
        auto mid = left;
        std::advance(mid, halfway);
        
        if ( *mid < value ) {
            left = ++mid;
            len -= halfway + 1;
        } else
            len = halfway;
    }
    
    return left;
}

/******************************************************************************/

// don't recompute the distance every time
// recursive version
template <typename ForwardIterator, typename T, typename L>
ForwardIterator lower_bound_recur_inner(ForwardIterator left, const T value, L len)
{
    if (len != 0)  {
        auto halfway = len / 2;
        
        auto mid = left;
        std::advance(mid, halfway);
        
        if ( *mid < value ) {
            left = ++mid;
            len -= halfway + 1;
            return lower_bound_recur_inner( left, value, len );
        } else {
            len = halfway;
            return lower_bound_recur_inner( left, value, len );
        }
    }
    
    return left;
}

template <typename ForwardIterator, typename T>
ForwardIterator lower_bound_recur(ForwardIterator left, ForwardIterator right, const T value)
{
    auto len = std::distance(left,right);
    return lower_bound_recur_inner(left,value,len);
}

/******************************************************************************/
/******************************************************************************/

// straightforward version, but slow for non-random access iterators
template <typename ForwardIterator, typename T>
ForwardIterator upper_bound1(ForwardIterator left, ForwardIterator right, const T value)
{
    while (left != right) {
        auto len = std::distance(left,right);
        auto halfway = len / 2;
        
        auto mid = left;
        std::advance(mid, halfway);
        
        if ( value < *mid )
            right = mid;
        else
            left = ++mid;
    }

    return left;
}

/******************************************************************************/

// don't recompute the distance every time
template <typename ForwardIterator, typename T>
ForwardIterator upper_bound2(ForwardIterator left, ForwardIterator right, const T value)
{
    auto len = std::distance(left,right);
    
    while (len != 0)  {
        auto halfway = len / 2;
        
        auto mid = left;
        std::advance(mid, halfway);
        
        if ( value < *mid )
            len = halfway;
        else {
            left = ++mid;
            len -= halfway + 1;
        }
    }
    
    return left;
}

/******************************************************************************/

// don't recompute the distance every time
// recursive version
template <typename ForwardIterator, typename T, typename L>
ForwardIterator upper_bound_recur_inner(ForwardIterator left, const T value, L len)
{
    if (len != 0)  {
        auto halfway = len / 2;
        
        auto mid = left;
        std::advance(mid, halfway);
        
        if ( value < *mid ) {
            len = halfway;
            return upper_bound_recur_inner( left, value, len );
        } else {
            left = ++mid;
            len -= halfway + 1;
            return upper_bound_recur_inner( left, value, len );
        }
    }
    
    return left;
}

template <typename ForwardIterator, typename T>
ForwardIterator upper_bound_recur(ForwardIterator left, ForwardIterator right, const T value)
{
    auto len = std::distance(left,right);
    return upper_bound_recur_inner(left,value,len);
}

/******************************************************************************/
/******************************************************************************/

// straightforward version, but slow for non-random access iterators
template <typename ForwardIterator, typename T>
bool binarysearch1(ForwardIterator left, ForwardIterator right, const T value)
{
    while (left != right) {
        auto len = std::distance(left,right);
        auto halfway = len / 2;
        
        auto mid = left;
        std::advance(mid, halfway);
        
        if ( *mid < value  )
            left = ++mid;
        else if ( *mid > value )
            right = mid;
        else
            return true;
    }

    return false;
}

/******************************************************************************/

// don't recompute the distance every time
template <typename ForwardIterator, typename T>
bool binarysearch2(ForwardIterator left, ForwardIterator right, const T value)
{
    auto len = std::distance(left,right);

    while (len != 0)  {
       auto halfway = len / 2;
        
        auto mid = left;
        std::advance(mid, halfway);
        
        if ( *mid < value ) {
            left = ++mid;
            len -= halfway + 1;
        } else if ( *mid > value )
            len = halfway;
        else
            return true;
    }
    
    return false;
}

/******************************************************************************/

// call lower bound and add logic to return bool - generally slower than explicit searches
template <typename ForwardIterator, typename T>
bool binarysearch3(ForwardIterator left, ForwardIterator right, const T value)
{
    auto result = lower_bound2( left, right, value );
    return (result != right && (*result == value) );
}

/******************************************************************************/

// don't recompute the distance every time
// try other small tweaks
template <typename ForwardIterator, typename T>
bool binarysearch4(ForwardIterator left, ForwardIterator right, const T value)
{
    auto len = std::distance(left,right);

    while (len > 0)  {
        auto halfway = len >> 1;        // len is always positive
        
        auto mid = left;
        std::advance(mid, halfway);
        
        auto midval = *mid;
        if ( midval < value ) {
            left = ++mid;
            len -= halfway + 1;
        } else if ( midval > value )
            len = halfway;
        else
            return true;
    }
    
    return false;
}

/******************************************************************************/

// don't recompute the distance every time
// recursive version
template <typename ForwardIterator, typename T, typename L>
bool binarysearch_recur_inner(ForwardIterator left, const T value, L len)
{
    if (len != 0)  {
        auto halfway = len / 2;
        
        auto mid = left;
        std::advance(mid, halfway);
        
        if ( *mid < value ) {
            left = ++mid;
            len -= halfway + 1;
            return binarysearch_recur_inner( left, value, len );
        } else if ( *mid > value ) {
            len = halfway;
            return binarysearch_recur_inner( left, value, len );
        } else
            return true;
    }
    
    return false;
}

template <typename ForwardIterator, typename T>
bool binarysearch_recur(ForwardIterator left, ForwardIterator right, const T value)
{
    auto len = std::distance(left,right);
    return binarysearch_recur_inner(left,value,len);
}

/******************************************************************************/
/******************************************************************************/

// give the compiler a hint
template<typename Iter, typename T>
Iter std_lowerbound(Iter begin, Iter end, const T value)
{
    return std::lower_bound(begin,end,value);
}

/******************************************************************************/

// give the compiler a hint
template<typename Iter, typename T>
Iter std_upperbound(Iter begin, Iter end, const T value)
{
    return std::upper_bound(begin,end,value);
}

/******************************************************************************/

// std::binary_search calls lower_bound and adds a small amount of logic to return a bool
// this is almost always slower than an explicit search

// give the compiler a hint
template<typename Iter, typename T>
bool std_binarysearch(Iter begin, Iter end, const T value)
{
    return std::binary_search(begin,end,value);
}

/******************************************************************************/
/******************************************************************************/

template<class Iterator>
Iterator medianOfThree( Iterator a, Iterator b, Iterator c )
{
    // insertion sort logic so result is stable
    if (*b < *a)
       swap(*a, *b);
    
    if (*c < *b) {
       swap(*b, *c);
       
       if (*b < *a)
          swap(*b, *a);
    }
    return b;
}

// far from perfect, but faster than "normal" forward iterator sorts for N > 20
template<class ForwardIterator>
void quicksort2_forward( ForwardIterator begin, ForwardIterator end )
{
    while ( end != begin ) {

        auto mm = begin;
        ++mm;
        if (mm == end)  // just one item
            return;
        
        auto ee = mm;
        ++ee;
        if (ee == end) {    // just two items
            if (*mm < *begin)
                swap(*mm,*begin);
            return;
        }

        auto middleValue = * medianOfThree( begin, mm, ee );
        
        auto right = ee;    // first location to test
        ++right;
        if (right == end)    // only three items, and we just sorted them
            return;
        
        auto left = mm;  // first location we might be able to swap a value into
        
        size_t leftSize = 1;

        // have to use a different partition scheme for forward iterators
        right = left;
        size_t rightSize = leftSize + 1;
        while ( ++right != end ) {
            if (*right < middleValue) {
                swap(*right,*left);
                ++left;
                ++leftSize;
            }
            ++rightSize;
        }

        // recurse smaller range, iterate on larger range
        // leftSize+rightSize == length
        rightSize -= leftSize;
        if ( rightSize < leftSize ) {
            quicksort2_forward( left, end );
            end = left;
        } else {
            quicksort2_forward( begin, left );
            begin = left;
        }
    }
}

/******************************************************************************/
/******************************************************************************/

typedef struct one_result {
    double time;
    size_t count;
    size_t iterations;
    std::string label;
    
    one_result(double t, size_t c, size_t ii, std::string ll) :
                time(t), count(c), iterations(ii), label(ll) {}
} one_result;

std::deque<one_result> gResults;

void record_result( double time, size_t count, size_t iterations, const std::string &label ) {
    gResults.push_back( one_result(time,count,iterations,label) );
}

/******************************************************************************/

void summarize(std::string name) {
    size_t i;
    double total_absolute_times = 0.0;
    const double timeThreshold = 1.0e-4;
    
    size_t current_test = gResults.size();
    
    if (current_test == 0)
        return;
    
    /* find longest label so we can adjust formatting
        12 = strlen("description")+1 */
    size_t longest_label_len = 12;
    for (i = 0; i < current_test; ++i) {
        size_t len = gResults[i].label.size();
        if (len > longest_label_len)
            longest_label_len = len;
    }
    
    printf("\ntest %*s description    absolute   operations      seconds\n", (int)longest_label_len-12, " ");
    printf("number %*s    time     per second    per operation\n\n", (int)longest_label_len, " ");
    
    for (i = 0; i < current_test; ++i) {
        
        // calculate total time
        total_absolute_times += gResults[i].time;
        
        // report as searches per second instead of increasing with size
//  150.0 to 0.0005 Mops/sec
//  8 to 1.7e6 nsec/op
        //double millions = ((double)(gResults[i].count) * gResults[i].iterations)/1.0e6;
        double millions = ((double)gResults[i].iterations)/1.0e6;
        
        double ops = 0.0;
        double speed = 1.0;
        if (gResults[i].time < timeThreshold) {
            speed = INFINITY;
            ops = 0.0;
        } else {
            speed = millions/gResults[i].time;
            ops = 1.0e9 * gResults[i].time / ((double)gResults[i].iterations);
        }
        
        printf("%3i %*s\"%s\"  %5.2f sec   %6.6g M    %6.6g nsec\n",
               (int)i,
               (int)(longest_label_len - gResults[i].label.size()),
               "",
               gResults[i].label.c_str(),
               gResults[i].time,
               speed,
               ops);
    }
    
    // report total time
    printf("\nTotal absolute time for %s: %.2f sec\n", name.c_str(), total_absolute_times);
    
    // reset the test counter so we can run more tests
    gResults.clear();
}

/******************************************************************************/
/******************************************************************************/

template < typename T, typename Iter >
bool searchDidFail( T result, Iter end )
{
    return (result == end);
}

template < typename Iter >
bool searchDidFail( bool result, Iter /* end */ )
{
    return !result;
}

/******************************************************************************/

template < typename Iter, typename SearchFunc >
void TestSearchArray( Iter begin, Iter end, size_t sequencesize, SearchFunc doSearch, const std::string label, bool isUpperBound = false )
{
    using T = typename std::iterator_traits<Iter>::value_type;

    const size_t max_iterations = 100000000;  // don't overflow, don't run forever
    size_t iterations = 0;
    double total_time = 0.0;
    bool failed = false;
    

    // use values randomly selected from the array
    scrand( sequencesize );
    const size_t valueTableSize = 1024;
    std::vector<T> valueList(valueTableSize);
    for (int i = 0; i < valueTableSize; ++i) {
        size_t index = crand64() % sequencesize;
        Iter temp = begin;
        std::advance(temp, index);
        valueList[i] = *temp;
    }
    
    
    typedef typename std::iterator_traits<Iter>::iterator_category iter_category;
    const bool isForwardIterator = std::is_convertible<iter_category, std::forward_iterator_tag>::value;
    const bool isBidirectionalIterator = std::is_convertible<iter_category, std::bidirectional_iterator_tag>::value;
    //const bool isRandomIterator = std::is_convertible<iter_category, std::random_access_iterator_tag>::value;
    
    // searches go too quickly and were being swamped by timer overhead.
    // So we sub-iterate to reduce overhead.
    // BUT remember that bidirectional/forward containers are SLOW.
    size_t tests = 0;
    int small_unroll = 50000;
    if (isBidirectionalIterator || isForwardIterator)
        small_unroll = 200;
    
    start_timer();
    do {
        for (int i=0; i < small_unroll; ++i) {
            T value = valueList[ iterations % valueTableSize ];
            ++iterations;
            auto result = doSearch( begin, end, value );
            if (!isUpperBound && searchDidFail(result, end) )
                failed = true;
        }
        total_time = timer();
        ++tests;
    } while ( (total_time < gMinimumTimeTarget) && (iterations < max_iterations) );

    //printf("size = %ld, iter = %ld, tests = %ld\n", sequencesize, iterations, tests );

    record_result( total_time, sequencesize, iterations, label );
    
    if (failed)
        printf("test %s failed\n", label.c_str());
    
}

/******************************************************************************/
/******************************************************************************/

template<typename Iter, typename Searcher>
void TestOneSearch( Iter begin, Iter end, size_t sequencesize, Searcher searchfunc, std::string label, bool isUpperBound = false )
{
    using T = typename std::iterator_traits<Iter>::value_type;

    benchmark::fill( begin, end, 5 );
    // single value, no chance of aliasing
    TestSearchArray( begin, end, sequencesize, searchfunc, label + " single_value", isUpperBound );

    fill_steps( begin, sequencesize, 10 );
    // counting to 10 won't have aliasing even with 8 bit values
    TestSearchArray( begin, end, sequencesize, searchfunc, label + " ten_values_ascending", isUpperBound );

    benchmark::fill_ascending( begin, end );
    quicksort2_forward(begin,end);       // deal with aliasing for smaller data sizes (8,16 bit)
    TestSearchArray( begin, end, sequencesize, searchfunc, label + " ascending", isUpperBound );
}

/******************************************************************************/
/******************************************************************************/

template<typename Iter>
void TestOneContainer(Iter begin, Iter end, size_t sequencesize, std::string label)
{
    using T = typename std::iterator_traits<Iter>::value_type;

    TestOneSearch( begin, end, sequencesize, std_lowerbound<Iter,T>, label + " std::lower_bound" );
    TestOneSearch( begin, end, sequencesize, lower_bound1<Iter,T>, label + " lower_bound1" );
    TestOneSearch( begin, end, sequencesize, lower_bound2<Iter,T>, label + " lower_bound2" );
    TestOneSearch( begin, end, sequencesize, lower_bound_recur<Iter,T>, label + " lower_bound_recursive" );
    
    
    TestOneSearch( begin, end, sequencesize, std_upperbound<Iter,T>, label + " std::upper_bound", true );
    TestOneSearch( begin, end, sequencesize, upper_bound1<Iter,T>, label + " upper_bound1", true );
    TestOneSearch( begin, end, sequencesize, upper_bound2<Iter,T>, label + " upper_bound2", true );
    TestOneSearch( begin, end, sequencesize, upper_bound_recur<Iter,T>, label + " upper_bound_recursive", true );
    
    
    TestOneSearch( begin, end, sequencesize, std_binarysearch<Iter,T>, label + " std::binary_search" );
    TestOneSearch( begin, end, sequencesize, binarysearch1<Iter,T>, label + " binary_search1" );
    TestOneSearch( begin, end, sequencesize, binarysearch2<Iter,T>, label + " binary_search2" );
    TestOneSearch( begin, end, sequencesize, binarysearch3<Iter,T>, label + " binary_search3" );
    TestOneSearch( begin, end, sequencesize, binarysearch4<Iter,T>, label + " binary_search4" );
    TestOneSearch( begin, end, sequencesize, binarysearch_recur<Iter,T>, label + " binary_search_recursive" );
    
    
    summarize( label + " Binary Search" );
}

/******************************************************************************/
/******************************************************************************/

template<typename T>
void TestOneType()
{
    // try to test trivial cases (mostly overhead) as well as cases in L1, L2, and DRAM
    size_t size_list[] = { 5, 10, 20, 50, 100, 200, 500, 1000, 2000, 5000, 10000, 20000, 50000, 100000, 200000  };   // release set
 //   size_t size_list[] = { 5, 10, 20, 50, 100, 200  }; // debug set
 //   size_t size_list[] = { 200, 2000, 20000, 200000  }; // debug set
    
    std::string myTypeName( getTypeName<T>() );
    

    for (auto sequencesize : size_list) {
        std::unique_ptr<T> arrayUP( new T[sequencesize] );
        TestOneContainer( arrayUP.get(), arrayUP.get()+sequencesize,
                         sequencesize, myTypeName + " " + std::to_string(sequencesize) + " pointer");
    }
    
    for (auto sequencesize : size_list) {
        std::vector<T> arrayVec(sequencesize);
        TestOneContainer( arrayVec.begin(), arrayVec.end(),
                         sequencesize, myTypeName + " " + std::to_string(sequencesize) +  " std::vector");
    }
    
    // std::array and std::vector test about the same on common compilers, so we'll skip array for now
    // See also stepanov_array.cpp and stepanov_vector.cpp
    
    for (auto sequencesize : size_list) {
        std::deque<T> arrayDeq(sequencesize);
        TestOneContainer( arrayDeq.begin(), arrayDeq.end(),
                         sequencesize, myTypeName + " " + std::to_string(sequencesize) +  " std::deque");
    }

    for (auto sequencesize : size_list) {
        std::list<T> arrayList(sequencesize);
        TestOneContainer( arrayList.begin(), arrayList.end(),
                         sequencesize, myTypeName + " " + std::to_string(sequencesize) +  " std::list");
    }

    for (auto sequencesize : size_list) {
        std::forward_list<T> arrayList(sequencesize);
        TestOneContainer( arrayList.begin(), arrayList.end(),
                         sequencesize, myTypeName + " " + std::to_string(sequencesize) +  " std::forward_list");
    }

}

/******************************************************************************/
/******************************************************************************/

int main(int argc, char* argv[])
{
    if (argc > 1)
        gMinimumTimeTarget = atof(argv[1]);
    
    
    // output command for documentation
    for (int i = 0; i < argc; ++i)
        printf("%s ", argv[i] );
    printf("\n");



    TestOneType<int8_t>();      // ends up with many repeated values for ascending
    TestOneType<uint16_t>();
    TestOneType<int32_t>();     // Few repeated values, good average case.
    TestOneType<float>();


#if WORKING_BUT_UNUSED
    TestOneType<uint8_t>();     // similar results to int8
    TestOneType<int16_t>();     // similar results to uint16
    TestOneType<uint32_t>();    // similar results to int32
    TestOneType<int64_t>();     // similar results to int32
    TestOneType<uint64_t>();    // similar results to int32
    TestOneType<double>();      // similar results to float
    TestOneType<long double>(); // poorly optimized by everyone
#endif

    return 0;
}

/******************************************************************************/
/******************************************************************************/
