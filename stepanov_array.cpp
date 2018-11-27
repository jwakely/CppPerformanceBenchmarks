/*
    Copyright 2007-2008 Adobe Systems Incorporated
    Copyright 2018 Chris Cox
    Distributed under the MIT License (see accompanying file LICENSE_1_0_0.txt
    or a copy at http://stlab.adobe.com/licenses.html )


Goal:  Examine any change in performance when moving from pointers to std::array iterators.


Assumptions:
    1) std::array iterators should not perform worse than raw pointers.
    
        Programmers should never be tempted(forced!) to write
            std::sort( &*vec.begin(), &*( vec.begin() + vec.size() ) )
        instead of
            std::sort( vec.begin(), vec.end() )


History:
    This is an extension to Alex Stepanov's original abstraction penalty benchmark
    to test the compiler vendor implementation of array iterators.


NOTE: So far results appear identical to stepanov_vector.cpp

*/

#include <cstddef>
#include <cstdio>
#include <ctime>
#include <cmath>
#include <cstdlib>
#include <array>
#include "benchmark_results.h"
#include "benchmark_timer.h"
#include "benchmark_algorithms.h"

/******************************************************************************/
/******************************************************************************/

// this constant may need to be adjusted to give reasonable minimum times
// For best results, times should be about 1.0 seconds for the minimum test run
int iterations = 3000000;

// 2000 items, or about 16k of data
// this is intended to remain within the L2 cache of most common CPUs
const int SIZE = 2000;

// initial value for filling our arrays, may be changed from the command line
double init_value = 2.0;

/******************************************************************************/
/******************************************************************************/

inline void check_sum(double result, const char *label) {
    if (result != SIZE * init_value)
        printf("test %s failed\n", label);
}

/******************************************************************************/

template <typename Iterator>
void verify_sorted(Iterator first, Iterator last, const char *label) {
    if (!benchmark::is_sorted(first,last))
        printf("sort test %s failed\n", label);
}

/******************************************************************************/

// a template using the accumulate template and iterators

template <typename Iterator, typename T>
void test_accumulate(Iterator first, Iterator last, T zero, const char *label) {
    int i;

    start_timer();

    for(i = 0; i < iterations; ++i)
        check_sum( double( accumulate(first, last, zero) ), label );

    record_result( timer(), label );
}

/******************************************************************************/

template <typename Iterator, typename T>
void test_insertion_sort(Iterator firstSource, Iterator lastSource, Iterator firstDest,
                        Iterator lastDest, T zero, const char *label) {
    int i;

    start_timer();

    for(i = 0; i < iterations; ++i) {
        ::copy(firstSource, lastSource, firstDest);
        insertionSort< Iterator, T>( firstDest, lastDest );
        verify_sorted( firstDest, lastDest, label );
    }
    
    record_result( timer(), label );
}

/******************************************************************************/

template <typename Iterator, typename T>
void test_quicksort(Iterator firstSource, Iterator lastSource, Iterator firstDest,
                    Iterator lastDest, T zero, const char *label) {
    int i;

    start_timer();

    for(i = 0; i < iterations; ++i) {
        std::copy(firstSource, lastSource, firstDest);
        quicksort< Iterator, T>( firstDest, lastDest );
        verify_sorted( firstDest, lastDest, label );
    }
    
    record_result( timer(), label );
}

/******************************************************************************/

template <typename Iterator, typename T>
void test_heap_sort(Iterator firstSource, Iterator lastSource, Iterator firstDest,
                    Iterator lastDest, T zero, const char *label) {
    int i;

    start_timer();

    for(i = 0; i < iterations; ++i) {
        ::copy(firstSource, lastSource, firstDest);
        heapsort< Iterator, T>( firstDest, lastDest );
        verify_sorted( firstDest, lastDest, label );
    }
    
    record_result( timer(), label );
}

/******************************************************************************/
/******************************************************************************/

// our global arrays of numbers to be summed

double data[SIZE];
double dataMaster[SIZE];

/******************************************************************************/

// declaration of our iterator types and begin/end pairs
typedef double* dp;
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

typedef std::array<double,SIZE>::iterator vdp;

typedef std::array<double,SIZE>::reverse_iterator rvdp;
typedef std::reverse_iterator< vdp > rtvdp;

typedef std::reverse_iterator< rvdp > rtrvdp;
typedef std::reverse_iterator< rtvdp > rtrtvdp;

/******************************************************************************/
/******************************************************************************/


int main(int argc, char** argv) {

    double dZero = 0.0;

    // output command for documentation:
    int i;
    for (i = 0; i < argc; ++i)
        printf("%s ", argv[i] );
    printf("\n");

    if (argc > 1) iterations = atoi(argv[1]);
    if (argc > 2) init_value = (double) atof(argv[2]);
    
    
    // seed the random number generator so we get repeatable results
    srand( (int)init_value + 123 );
    

    ::fill(dpb, dpe, double(init_value));
    
    std::array<double,SIZE>   vec_data;

    ::fill(vec_data.begin(), vec_data.end(), double(init_value));
    
    rtvdp rtvdpb(vec_data.end());
    rtvdp rtvdpe(vec_data.begin());
    
    rtrvdp rtrvdpb(vec_data.rend());
    rtrvdp rtrvdpe(vec_data.rbegin());
    
    rtrtvdp rtrtvdpb(rtvdpe);
    rtrtvdp rtrtvdpe(rtvdpb);

    test_accumulate(dpb, dpe, dZero, "accumulate double pointer verify3");
    test_accumulate(vec_data.begin(), vec_data.end(), dZero, "accumulate double array iterator");
    test_accumulate(rdpb, rdpe, dZero, "accumulate double pointer reverse");
    test_accumulate(vec_data.rbegin(), vec_data.rend(), dZero, "accumulate double array reverse_iterator");
    test_accumulate(rtvdpb, rtvdpe, dZero, "accumulate double array iterator reverse");
    test_accumulate(rrdpb, rrdpe, dZero, "accumulate double pointer reverse reverse");
    test_accumulate(rtrvdpb, rtrvdpe, dZero, "accumulate double array reverse_iterator reverse");
    test_accumulate(rtrtvdpb, rtrtvdpe, dZero, "accumulate double array iterator reverse reverse");

    summarize("Array accumulate", SIZE, iterations, kShowGMeans, kShowPenalty );



    // the sorting tests are much slower than the accumulation tests - O(N^2)
    iterations = iterations / 2000;
    
    std::array<double,SIZE>   vec_dataMaster;
    
    // fill one set of random numbers
    fill_random<double *, double>( dMpb, dMpe );
    
    // copy to the other sets, so we have the same numbers
    ::copy( dMpb, dMpe, vec_dataMaster.begin() );
    
    rtvdp rtvdMpb(vec_dataMaster.end());
    rtvdp rtvdMpe(vec_dataMaster.begin());
    
    rtrvdp rtrvdMpb(vec_dataMaster.rend());
    rtrvdp rtrvdMpe(vec_dataMaster.rbegin());
    
    rtrtvdp rtrtvdMpb(rtvdMpe);
    rtrtvdp rtrtvdMpe(rtvdMpb);

    test_insertion_sort(dMpb, dMpe, dpb, dpe, dZero, "insertion_sort double pointer verify3");
    test_insertion_sort(vec_dataMaster.begin(), vec_dataMaster.end(), vec_data.begin(), vec_data.end(), dZero, "insertion_sort double array iterator");
    test_insertion_sort(rdMpb, rdMpe, rdpb, rdpe, dZero, "insertion_sort double pointer reverse");
    test_insertion_sort(vec_dataMaster.rbegin(), vec_dataMaster.rend(), vec_data.rbegin(), vec_data.rend(), dZero, "insertion_sort double array reverse_iterator");
    test_insertion_sort(rtvdMpb, rtvdMpe, rtvdpb, rtvdpe, dZero, "insertion_sort double array iterator reverse");
    test_insertion_sort(rrdMpb, rrdMpe, rrdpb, rrdpe, dZero, "insertion_sort double pointer reverse reverse");
    test_insertion_sort(rtrvdMpb, rtrvdMpe, rtrvdpb, rtrvdpe, dZero, "insertion_sort double array reverse_iterator reverse");
    test_insertion_sort(rtrtvdMpb, rtrtvdMpe, rtrtvdpb, rtrtvdpe, dZero, "insertion_sort double array iterator reverse reverse");

    summarize("Array Insertion Sort", SIZE, iterations, kShowGMeans, kShowPenalty );

    
    // these are slightly faster - O(NLog2(N))
    iterations = iterations * 16;
    
    test_quicksort(dMpb, dMpe, dpb, dpe, dZero, "quicksort double pointer verify3");
    test_quicksort(vec_dataMaster.begin(), vec_dataMaster.end(), vec_data.begin(), vec_data.end(), dZero, "quicksort double array iterator");
    test_quicksort(rdMpb, rdMpe, rdpb, rdpe, dZero, "quicksort double pointer reverse");
    test_quicksort(vec_dataMaster.rbegin(), vec_dataMaster.rend(), vec_data.rbegin(), vec_data.rend(), dZero, "quicksort double array reverse_iterator");
    test_quicksort(rtvdMpb, rtvdMpe, rtvdpb, rtvdpe, dZero, "quicksort double array iterator reverse");
    test_quicksort(rrdMpb, rrdMpe, rrdpb, rrdpe, dZero, "quicksort double pointer reverse reverse");
    test_quicksort(rtrvdMpb, rtrvdMpe, rtrvdpb, rtrvdpe, dZero, "quicksort double array reverse_iterator reverse");
    test_quicksort(rtrtvdMpb, rtrtvdMpe, rtrtvdpb, rtrtvdpe, dZero, "quicksort double array iterator reverse reverse");

    summarize("Array Quicksort", SIZE, iterations, kShowGMeans, kShowPenalty );

    
    test_heap_sort(dMpb, dMpe, dpb, dpe, dZero, "heap_sort double pointer verify3");
    test_heap_sort(vec_dataMaster.begin(), vec_dataMaster.end(), vec_data.begin(), vec_data.end(), dZero, "heap_sort double array iterator");
    test_heap_sort(rdMpb, rdMpe, rdpb, rdpe, dZero, "heap_sort double pointer reverse");
    test_heap_sort(vec_dataMaster.rbegin(), vec_dataMaster.rend(), vec_data.rbegin(), vec_data.rend(), dZero, "heap_sort double array reverse_iterator");
    test_heap_sort(rtvdMpb, rtvdMpe, rtvdpb, rtvdpe, dZero, "heap_sort double array iterator reverse");
    test_heap_sort(rrdMpb, rrdMpe, rrdpb, rrdpe, dZero, "heap_sort double pointer reverse reverse");
    test_heap_sort(rtrvdMpb, rtrvdMpe, rtrvdpb, rtrvdpe, dZero, "heap_sort double array reverse_iterator reverse");
    test_heap_sort(rtrtvdMpb, rtrtvdMpe, rtrtvdpb, rtrtvdpe, dZero, "heap_sort double array iterator reverse reverse");

    summarize("Array Heap Sort", SIZE, iterations, kShowGMeans, kShowPenalty );



    return 0;
}

// the end
/******************************************************************************/
/******************************************************************************/
