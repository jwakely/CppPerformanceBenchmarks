/*
    Copyright 2007-2008 Adobe Systems Incorporated
    Copyright 2018-2019 Chris Cox
    Distributed under the MIT License (see accompanying file LICENSE_1_0_0.txt
    or a copy at http://stlab.adobe.com/licenses.html )


Goal:  Examine any change in performance when moving from pointers to vector iterators.


Assumptions:
    1) std::vector iterators should not perform worse than raw pointers.
    
        Programmers should never be tempted (forced!) to write
            std::sort( &*vec.begin(), &*( vec.begin() + vec.size() ) )
        instead of
            std::sort( vec.begin(), vec.end() )

    2) iterators reversed twice should not perform worse than raw iterators.


History:
    This is an extension to Alex Stepanov's original abstraction penalty benchmark
    to test the compiler vendor implementation of vector iterators.


*/

#include <cstddef>
#include <cstdio>
#include <ctime>
#include <cmath>
#include <cstdlib>
#include <vector>
#include <string>
#include <deque>
#include "benchmark_results.h"
#include "benchmark_timer.h"
#include "benchmark_algorithms.h"
#include "benchmark_typenames.h"

/******************************************************************************/
/******************************************************************************/

// this constant may need to be adjusted to give reasonable minimum times
// For best results, times should be about 1.0 seconds for the minimum test run
int iterations = 2200000;

// 2000 items, or about 16k of data
// this is intended to remain within the L2 cache of most common CPUs
const int SIZE = 2000;

// initial value for filling our arrays, may be changed from the command line
double init_value = 3.0;

/******************************************************************************/
/******************************************************************************/

// reverse iterator templates

template <class RandomAccessIterator, class T>
struct reverse_iterator {
    RandomAccessIterator current;
    
    reverse_iterator(RandomAccessIterator x) : current(x) {}
    
    T& operator*() const {
        RandomAccessIterator tmp = current;
        return *(--tmp);
    }
    
    reverse_iterator<RandomAccessIterator, T>& operator++() {
        --current;
        return *this;
    }
    reverse_iterator<RandomAccessIterator, T> operator++(int) {
      reverse_iterator<RandomAccessIterator, T> tmp = *this;
        ++*this;
        return tmp;
    }
    reverse_iterator<RandomAccessIterator, T>& operator--() {
        ++current;
        return *this;
    }
    reverse_iterator<RandomAccessIterator, T> operator--(int) {
      reverse_iterator<RandomAccessIterator, T> tmp = *this;
        --*this;
        return tmp;
    }
};

template <class RandomAccessIterator, class T>
inline int operator==(const reverse_iterator<RandomAccessIterator, T>& x,
             const reverse_iterator<RandomAccessIterator, T>& y) {
    return x.current == y.current;
}

template <class RandomAccessIterator, class T>
inline int operator!=(const reverse_iterator<RandomAccessIterator, T>& x,
              const reverse_iterator<RandomAccessIterator, T>& y) {
    return x.current != y.current;
}


// really a distance between pointers, which must return ptrdiff_t
// because (ptr - ptr) --> ptrdiff_t
template <class RandomAccessIterator, class T>
inline ptrdiff_t operator-(reverse_iterator<RandomAccessIterator, T>& xx, reverse_iterator<RandomAccessIterator, T>& yy) {
    return (ptrdiff_t)( xx.current - yy.current );
}

template <class RandomAccessIterator, class T>
inline reverse_iterator<RandomAccessIterator, T> operator-(reverse_iterator<RandomAccessIterator, T> &xx, ptrdiff_t inc) {
    reverse_iterator<RandomAccessIterator, T> tmp = xx;
    tmp.current += inc;
    return tmp;
}

template <class RandomAccessIterator, class T>
inline reverse_iterator<RandomAccessIterator, T> operator+(reverse_iterator<RandomAccessIterator, T> &xx, ptrdiff_t inc) {
    reverse_iterator<RandomAccessIterator, T> tmp = xx;
    tmp.current -= inc;
    return tmp;
}

template <class RandomAccessIterator, class T>
inline reverse_iterator<RandomAccessIterator, T>& operator+=(reverse_iterator<RandomAccessIterator, T> &xx, ptrdiff_t inc) {
    xx.current -= inc;
    return xx;
}

template <class RandomAccessIterator, class T>
inline reverse_iterator<RandomAccessIterator, T>& operator-=(reverse_iterator<RandomAccessIterator, T> &xx, ptrdiff_t inc) {
    xx.current += inc;
    return xx;
}

template <class RandomAccessIterator, class T>
inline bool operator<(const reverse_iterator<RandomAccessIterator, T>& x, const reverse_iterator<RandomAccessIterator, T>& y) {
    return (x.current < y.current);
}

/******************************************************************************/
/******************************************************************************/

template <typename T>
inline void check_sum(T result, const std::string &label) {
    if (result != T(SIZE * init_value))
        printf("test %s failed\n", label.c_str() );
}

/******************************************************************************/

template <typename Iterator>
void verify_sorted(Iterator first, Iterator last, const std::string &label) {
    if (!benchmark::is_sorted(first,last))
        printf("sort test %s failed\n", label.c_str() );
}

/******************************************************************************/

static std::deque<std::string> gLabels;

void record_std_result( double time, const std::string &label ) {
  gLabels.push_back( label );
  record_result( time, gLabels.back().c_str() );
}

/******************************************************************************/

// a template using the accumulate template and iterators

template <typename Iterator, typename T>
void test_accumulate(Iterator first, Iterator last, T zero, const std::string label) {
    int i;

    start_timer();

    for(i = 0; i < iterations; ++i)
        check_sum( T( benchmark::accumulate(first, last, zero) ), label );

    record_std_result( timer(), label );
}

/******************************************************************************/

template <typename Iterator, typename T>
void test_insertion_sort(Iterator firstSource, Iterator lastSource, Iterator firstDest,
                        Iterator lastDest, T zero, const std::string label) {
    int i;

    start_timer();

    for(i = 0; i < iterations; ++i) {
        ::copy(firstSource, lastSource, firstDest);
        insertionSort( firstDest, lastDest );
        verify_sorted( firstDest, lastDest, label );
    }
    
    record_std_result( timer(), label );
}

/******************************************************************************/

template <typename Iterator, typename T>
void test_quicksort(Iterator firstSource, Iterator lastSource, Iterator firstDest,
                    Iterator lastDest, T zero, const std::string label) {
    int i;

    start_timer();

    for(i = 0; i < iterations; ++i) {
        ::copy(firstSource, lastSource, firstDest);
        quicksort( firstDest, lastDest );
        verify_sorted( firstDest, lastDest, label );
    }
    
    record_std_result( timer(), label );
}

/******************************************************************************/

template <typename Iterator, typename T>
void test_heap_sort(Iterator firstSource, Iterator lastSource, Iterator firstDest,
                    Iterator lastDest, T zero, const std::string label) {
    int i;

    start_timer();

    for(i = 0; i < iterations; ++i) {
        ::copy(firstSource, lastSource, firstDest);
        heapsort( firstDest, lastDest );
        verify_sorted( firstDest, lastDest, label );
    }
    
    record_std_result( timer(), label );
}

/******************************************************************************/
/******************************************************************************/


template< typename T>
void TestOneType()
{
    // our arrays of numbers to operate on
    T data[SIZE];
    T dataMaster[SIZE];

    // declaration of our iterator types and begin/end pairs
    typedef T* dp;
    dp dpb = data;
    dp dpe = data + SIZE;
    dp dMpb = dataMaster;
    dp dMpe = dataMaster + SIZE;

    typedef std::reverse_iterator<dp> rdp;
    rdp rdpb(dpe);
    rdp rdpe(dpb);
    rdp rdMpb(dMpe);
    rdp rdMpe(dMpb);

    typedef std::reverse_iterator<rdp> rrdp;
    rrdp rrdpb(rdpe);
    rrdp rrdpe(rdpb);
    rrdp rrdMpb(rdMpe);
    rrdp rrdMpe(rdMpb);

    typedef std::vector<T>  typeVector;
    typedef typename typeVector::iterator vdp;
    typedef typename typeVector::reverse_iterator rvdp;
    typedef std::reverse_iterator< vdp > rtvdp;
    typedef reverse_iterator< vdp, T > Rtvdp;

    typedef std::reverse_iterator< rvdp > rtrvdp;
    typedef std::reverse_iterator< rtvdp > rtrtvdp;
    typedef reverse_iterator< Rtvdp, T > Rtrtvdp;

    T dZero = 0.0;
    
    
    std::string myTypeName( getTypeName<T>() );
    
    gLabels.clear();
    
    int base_iterations = iterations;
    
    // seed the random number generator so we get repeatable results
    scrand( (int)init_value + 234 );
    
    ::fill(dpb, dpe, T(init_value));
    
    typeVector   vec_data;
    vec_data.resize(SIZE);

    ::fill(vec_data.begin(), vec_data.end(), T(init_value));
    
    rtvdp rtvdpb(vec_data.end());
    rtvdp rtvdpe(vec_data.begin());
    
    rtrvdp rtrvdpb(vec_data.rend());
    rtrvdp rtrvdpe(vec_data.rbegin());
    
    rtrtvdp rtrtvdpb(rtvdpe);
    rtrtvdp rtrtvdpe(rtvdpb);
    
    Rtvdp Rtvdpb(vec_data.end());
    Rtvdp Rtvdpe(vec_data.begin());
    
    Rtrtvdp Rtrtvdpb( Rtvdpe );
    Rtrtvdp Rtrtvdpe( Rtvdpb );

    test_accumulate(dpb, dpe, dZero, myTypeName + " accumulate pointer verify2");
    test_accumulate(vec_data.begin(), vec_data.end(), dZero, myTypeName + " accumulate vector iterator");
    test_accumulate(rrdpb, rrdpe, dZero, myTypeName + " accumulate pointer reverse reverse");
    test_accumulate(rtrvdpb, rtrvdpe, dZero, myTypeName + " accumulate vector reverse_iterator reverse");
    test_accumulate(rtrtvdpb, rtrtvdpe, dZero, myTypeName + " accumulate vector iterator reverse reverse");
    test_accumulate(Rtrtvdpb, Rtrtvdpe, dZero, myTypeName + " accumulate array Riterator reverse reverse");

    std::string temp1( myTypeName + " Vector Accumulate");
    summarize( temp1.c_str(), SIZE, iterations, kShowGMeans, kShowPenalty );



    // the sorting tests are much slower than the accumulation tests - O(N^2)
    iterations = iterations / 2000;
    
    std::vector<T>   vec_dataMaster;
    vec_dataMaster.resize(SIZE);
    
    // fill one set of random numbers
    fill_random( dMpb, dMpe );
    
    // copy to the other sets, so we have the same numbers
    ::copy( dMpb, dMpe, vec_dataMaster.begin() );
    
    rtvdp rtvdMpb(vec_dataMaster.end());
    rtvdp rtvdMpe(vec_dataMaster.begin());
    
    rtrvdp rtrvdMpb(vec_dataMaster.rend());
    rtrvdp rtrvdMpe(vec_dataMaster.rbegin());
    
    rtrtvdp rtrtvdMpb(rtvdMpe);
    rtrtvdp rtrtvdMpe(rtvdMpb);
    
    Rtvdp RtvdMpb(vec_dataMaster.end());
    Rtvdp RtvdMpe(vec_dataMaster.begin());
    
    Rtrtvdp RtrtvdMpb( RtvdMpe );
    Rtrtvdp RtrtvdMpe( RtvdMpb );

    test_insertion_sort(dMpb, dMpe, dpb, dpe, dZero, myTypeName + " insertion_sort pointer verify2");
    test_insertion_sort(vec_dataMaster.begin(), vec_dataMaster.end(), vec_data.begin(), vec_data.end(), dZero, myTypeName + " insertion_sort vector iterator");
    test_insertion_sort(rrdMpb, rrdMpe, rrdpb, rrdpe, dZero, myTypeName + " insertion_sort pointer reverse reverse");
    test_insertion_sort(rtrvdMpb, rtrvdMpe, rtrvdpb, rtrvdpe, dZero, myTypeName + " insertion_sort vector reverse_iterator reverse");
    test_insertion_sort(rtrtvdMpb, rtrtvdMpe, rtrtvdpb, rtrtvdpe, dZero, myTypeName + " insertion_sort vector iterator reverse reverse");
    test_insertion_sort(RtrtvdMpb, RtrtvdMpe, Rtrtvdpb, Rtrtvdpe, dZero, myTypeName + " insertion_sort array Riterator reverse reverse");

    std::string temp2( myTypeName + " Vector Insertion Sort");
    summarize( temp2.c_str(), SIZE, iterations, kShowGMeans, kShowPenalty );

    
    // these are slightly faster - O(NLog2(N))
    iterations = iterations * 8;
    
    test_quicksort(dMpb, dMpe, dpb, dpe, dZero, myTypeName + " quicksort pointer verify2");
    test_quicksort(vec_dataMaster.begin(), vec_dataMaster.end(), vec_data.begin(), vec_data.end(), dZero, myTypeName + " quicksort vector iterator");
    test_quicksort(rrdMpb, rrdMpe, rrdpb, rrdpe, dZero, myTypeName + " quicksort pointer reverse reverse");
    test_quicksort(rtrvdMpb, rtrvdMpe, rtrvdpb, rtrvdpe, dZero, myTypeName + " quicksort vector reverse_iterator reverse");
    test_quicksort(rtrtvdMpb, rtrtvdMpe, rtrtvdpb, rtrtvdpe, dZero, myTypeName + " quicksort vector iterator reverse reverse");
    test_quicksort(RtrtvdMpb, RtrtvdMpe, Rtrtvdpb, Rtrtvdpe, dZero, myTypeName + " quicksort array Riterator reverse reverse");

    std::string temp3( myTypeName + " Vector Quicksort");
    summarize( temp3.c_str(), SIZE, iterations, kShowGMeans, kShowPenalty );

    
    test_heap_sort(dMpb, dMpe, dpb, dpe, dZero, myTypeName + " heap_sort pointer verify2");
    test_heap_sort(vec_dataMaster.begin(), vec_dataMaster.end(), vec_data.begin(), vec_data.end(), dZero, myTypeName + " heap_sort vector iterator");
    test_heap_sort(rrdMpb, rrdMpe, rrdpb, rrdpe, dZero, myTypeName + " heap_sort pointer reverse reverse");
    test_heap_sort(rtrvdMpb, rtrvdMpe, rtrvdpb, rtrvdpe, dZero, myTypeName + " heap_sort vector reverse_iterator reverse");
    test_heap_sort(rtrtvdMpb, rtrtvdMpe, rtrtvdpb, rtrtvdpe, dZero, myTypeName + " heap_sort vector iterator reverse reverse");
    test_heap_sort(RtrtvdMpb, RtrtvdMpe, Rtrtvdpb, Rtrtvdpe, dZero, myTypeName + " heap_sort array Riterator reverse reverse");
    
    std::string temp4( myTypeName + " Vector Heap Sort");
    summarize( temp4.c_str(), SIZE, iterations, kShowGMeans, kShowPenalty );


    iterations = base_iterations;
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
    

    // the classic
    TestOneType<double>();

#if THESE_WORK_BUT_ARE_NOT_NEEDED_YET
    TestOneType<float>();
    TestOneType<long double>();     // this isn't well optimized overall yet
#endif


    iterations *= 3;
    TestOneType<int32_t>();
    TestOneType<uint64_t>();

#if THESE_WORK_BUT_ARE_NOT_NEEDED_YET
    TestOneType<int8_t>();
    TestOneType<uint8_t>();
    TestOneType<int16_t>();
    TestOneType<uint16_t>();
    TestOneType<uint32_t>();
    TestOneType<int64_t>();
#endif


    return 0;
}

// the end
/******************************************************************************/
/******************************************************************************/
