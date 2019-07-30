/*
    Copyright 2007-2008 Adobe Systems Incorporated
    Copyright 2018-2019 Chris Cox
    Distributed under the MIT License (see accompanying file LICENSE_1_0_0.txt
    or a copy at http://stlab.adobe.com/licenses.html )


Goal:  Examine any change in performance when adding abstraction to simple data types.
    In other words:  what happens when adding {} around a type?


Assumptions:
    
    1) A value wrapped in a struct or class should not perform worse than a raw value.
    
    2) A value recursively wrapped in a struct or class should not perform worse than the raw value.

    3) An iterator reversed twice should not perform worse than a plain iterator.
            Assumes that basic algebraic reduction works correctly.


History:
    Alex Stepanov created the abstraction penalty benchmark.
    Recently, Alex suggested that I take ownership of his benchmark and extend it.
    
    The original accumulation tests used to show large penalties for using abstraction,
    but compilers have improved over time.  I have added three sorting tests
    with non-trivial value and pointer usage that show that some compilers still
    have more opportunities for optimization.
    
    Chris Cox
    February 2008

*/

#include <cstddef>
#include <cstdio>
#include <ctime>
#include <cmath>
#include <cstdlib>
#include <string>
#include <deque>
#include "benchmark_results.h"
#include "benchmark_timer.h"
#include "benchmark_algorithms.h"
#include "benchmark_typenames.h"

/******************************************************************************/

// A value wrapped in a struct, possibly recursively

template <typename T>
struct ValueWrapper {
    T value;
    ValueWrapper() {}
    template<typename TT>
        inline operator TT () const { return (TT)value; }
    template<typename TT>
        ValueWrapper(const TT& x) : value(x) {}
    
    T& operator*() const { return *value; }
};

template <typename T>
inline ValueWrapper<T> operator+(const ValueWrapper<T>& x, const ValueWrapper<T>& y) {
    return ValueWrapper<T>(x.value + y.value);
}

template <typename T>
inline bool operator<(const ValueWrapper<T>& x, const ValueWrapper<T>& y) {
    return (x.value < y.value);
}

template <typename T>
inline bool operator!=(const ValueWrapper<T>& x, const ValueWrapper<T>& y) {
    return (x.value != y.value);
}

template <typename T>
inline bool operator==(const ValueWrapper<T>& x, const ValueWrapper<T>& y) {
    return (x.value == y.value);
}

/******************************************************************************/

// A value wrapped in a struct, possibly recursively
// with differences in struct to make it distinct from above struct

template <typename T>
struct ValueWrapper2 {
    T value;
    ValueWrapper2() {}
    template<typename TT>
        inline operator TT () const { return (TT)value; }
    template<typename TT>
        ValueWrapper2(const TT& x) : value(x) {}
    T& operator*() const { return *value; }
    
    operator bool() const { return value != T(0); }

    // this function is unused in this test - this is just to add something beyond the above class
    inline ValueWrapper2 do_nothing (const ValueWrapper2& x, const ValueWrapper2& y) {
        return ValueWrapper2(x.value * y.value);
    }

};

template <typename T>
inline ValueWrapper2<T> operator+(const ValueWrapper2<T>& x, const ValueWrapper2<T>& y) {
    return ValueWrapper2<T>(x.value + y.value);
}

template <typename T>
inline bool operator<(const ValueWrapper2<T>& x, const ValueWrapper2<T>& y) {
    return (x.value < y.value);
}

template <typename T>
inline bool operator!=(const ValueWrapper2<T>& x, const ValueWrapper2<T>& y) {
    return (x.value != y.value);
}

template <typename T>
inline bool operator==(const ValueWrapper2<T>& x, const ValueWrapper2<T>& y) {
    return (x.value == y.value);
}

/******************************************************************************/

// a pointer wrapped in a struct, aka an iterator

template<typename T>
struct PointerWrapper {
    T* current;
    PointerWrapper() {}
    PointerWrapper(T* x) : current(x) {}
    T& operator*() const { return *current; }
};

// really a distance between pointers, which must return ptrdiff_t
// because (ptr - ptr) --> ptrdiff_t
template <typename T>
inline ptrdiff_t operator-(PointerWrapper<T>& xx, PointerWrapper<T>& yy) {
    return (ptrdiff_t)( xx.current - yy.current );
}

template <typename T>
inline PointerWrapper<T>& operator++(PointerWrapper<T> &xx) {
    ++xx.current;
    return xx;
}

template <typename T>
inline PointerWrapper<T>& operator--(PointerWrapper<T> &xx) {
    --xx.current;
    return xx;
}

template <typename T>
inline PointerWrapper<T> operator++(PointerWrapper<T> &xx, int) {
    PointerWrapper<T> tmp = xx;
    ++xx;
    return tmp;
}

template <typename T>
inline PointerWrapper<T> operator--(PointerWrapper<T> &xx, int) {
    PointerWrapper<T> tmp = xx;
    --xx;
    return tmp;
}

template <typename T>
inline PointerWrapper<T> operator-(PointerWrapper<T> &xx, ptrdiff_t inc) {
    PointerWrapper<T> tmp = xx;
    tmp.current -= inc;
    return tmp;
}

template <typename T>
inline PointerWrapper<T> operator+(PointerWrapper<T> &xx, ptrdiff_t inc) {
    PointerWrapper<T> tmp = xx;
    tmp.current += inc;
    return tmp;
}

template <typename T>
inline PointerWrapper<T>& operator+=(PointerWrapper<T> &xx, ptrdiff_t inc) {
    xx.current += inc;
    return xx;
}

template <typename T>
inline PointerWrapper<T>& operator-=(PointerWrapper<T> &xx, ptrdiff_t inc) {
    xx.current -= inc;
    return xx;
}

template <typename T>
inline bool operator<(const PointerWrapper<T>& x, const PointerWrapper<T>& y) {
    return (x.current < y.current);
}

template <typename T>
inline bool operator==(const PointerWrapper<T>& x, const PointerWrapper<T>& y) {
    return (x.current == y.current);
}

template <typename T>
inline bool operator!=(const PointerWrapper<T>& x, const PointerWrapper<T>& y) {
    return (x.current != y.current);
}

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

// this constant may need to be adjusted to give reasonable minimum times
// For best results, times should be about 1.0 seconds for the minimum test run
int iterations = 2000000;

// 2000 items, or about 16k of data
// this is intended to remain within the L2 cache of most common CPUs
const int SIZE = 2000;

// initial value for filling our arrays, may be changed from the command line
double init_value = 3.0;

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
    if (!is_sorted(first,last))
        printf("sort test %s failed\n", label.c_str() );
}

/******************************************************************************/

static std::deque<std::string> gLabels;

void record_std_result( double time, const std::string &label ) {
  gLabels.push_back( label );
  record_result( time, gLabels.back().c_str() );
}

/******************************************************************************/

template <typename Iterator, typename T>
void test_accumulate(Iterator first, Iterator last, T zero, const std::string label) {
  int i;
  
  start_timer();
  
  for(i = 0; i < iterations; ++i)
    check_sum( T( accumulate(first, last, zero) ), label );
    
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
    typedef ValueWrapper<T>    TypeValueWrapper;
    typedef ValueWrapper< ValueWrapper< ValueWrapper< ValueWrapper< ValueWrapper< ValueWrapper< ValueWrapper< ValueWrapper< ValueWrapper< ValueWrapper<T> > > > > > > > > > TypeValueWrapper10;

    typedef reverse_iterator< reverse_iterator< T *, T>, T > type_RRpointer;
    typedef reverse_iterator< reverse_iterator< PointerWrapper<T>, T>, T> typeWrapper_RRpointer;
    typedef reverse_iterator< reverse_iterator< PointerWrapper<TypeValueWrapper>, TypeValueWrapper>, TypeValueWrapper> typeWrapper_RRWrapperPointer;

    typedef PointerWrapper<T> type_pointer;
    typedef PointerWrapper<TypeValueWrapper> typeValueWrapper_pointer;
    typedef PointerWrapper<TypeValueWrapper10> typeValueWrapper10_pointer;

    // our arrays of numbers to operate on
    T data[SIZE];
    TypeValueWrapper VData[SIZE];
    TypeValueWrapper10 V10Data[SIZE];

    T dataMaster[SIZE];
    TypeValueWrapper VDataMaster[SIZE];
    TypeValueWrapper10 V10DataMaster[SIZE];


    // declaration of our iterator types and begin/end pairs
    typedef T* dp;
    dp dpb = data;
    dp dpe = data + SIZE;
    dp dMpb = dataMaster;
    dp dMpe = dataMaster + SIZE;

    typedef TypeValueWrapper* DVp;
    DVp DVpb = VData;
    DVp DVpe = VData + SIZE;
    DVp DVMpb = VDataMaster;
    DVp DVMpe = VDataMaster + SIZE;

    typedef TypeValueWrapper10* DV10p;
    DV10p DV10pb = V10Data;
    DV10p DV10pe = V10Data + SIZE;
    DV10p DV10Mpb = V10DataMaster;
    DV10p DV10Mpe = V10DataMaster + SIZE;

    typedef type_pointer dP;
    dP dPb(dpb);
    dP dPe(dpe);
    dP dMPb(dMpb);
    dP dMPe(dMpe);

    typedef typeValueWrapper_pointer DVP;
    DVP DVPb(DVpb);
    DVP DVPe(DVpe);
    DVP DVMPb(DVMpb);
    DVP DVMPe(DVMpe);

    typedef typeValueWrapper10_pointer DV10P;
    DV10P DV10Pb(DV10pb);
    DV10P DV10Pe(DV10pe);
    DV10P DV10MPb(DV10Mpb);
    DV10P DV10MPe(DV10Mpe);

    typedef type_RRpointer dRRp;
    dRRp dRRpb( data );
    dRRp dRRpe( data + SIZE );
    dRRp dRRMpb( dataMaster );
    dRRp dRRMpe( dataMaster + SIZE );

    typedef typeWrapper_RRpointer dRRP;
    dRRP dRRPb( dPb );
    dRRP dRRPe( dPe );
    dRRP dRRMPb( dMPb );
    dRRP dRRMPe( dMPe );

    typedef typeWrapper_RRWrapperPointer DVRRP;
    DVRRP DVRRPb( DVPb );
    DVRRP DVRRPe( DVPe );
    DVRRP DVRRMPb( DVMPb );
    DVRRP DVRRMPe( DVMPe );


    // zero init values for accumulation
    T dZero = 0.0;
    TypeValueWrapper DVZero = 0.0;
    TypeValueWrapper10 DV10Zero = TypeValueWrapper10(0.0);


    std::string myTypeName( getTypeName<T>() );
    
    gLabels.clear();
    
    int base_iterations = iterations;

    // seed the random number generator so we get repeatable results
    scrand( (int)init_value + 123 );

    fill(dpb, dpe, T(init_value));
    fill(DVpb, DVpe, TypeValueWrapper(init_value));
    fill(DV10pb, DV10pe, TypeValueWrapper10(init_value));

    test_accumulate(dpb, dpe, dZero, myTypeName + " accumulate pointer");
    test_accumulate(dPb, dPe, dZero, myTypeName + " accumulate pointer_class");
    test_accumulate(DVpb, DVpe, DVZero, myTypeName + " accumulate TypeValueWrapper pointer");
    test_accumulate(DVPb, DVPe, DVZero, myTypeName + " accumulate TypeValueWrapper pointer_class");
    test_accumulate(DV10pb, DV10pe, DV10Zero, myTypeName + " accumulate TypeValueWrapper10 pointer");
    test_accumulate(DV10Pb, DV10Pe, DV10Zero, myTypeName + " accumulate TypeValueWrapper10 pointer_class");
    test_accumulate(dRRpb, dRRpe, dZero, myTypeName + " accumulate reverse reverse pointer");
    test_accumulate(dRRPb, dRRPe, dZero, myTypeName + " accumulate reverse reverse pointer_class");
    test_accumulate(DVRRPb, DVRRPe, DVZero, myTypeName + " accumulate TypeValueWrapper reverse reverse pointer_class");

    std::string temp1( myTypeName + " Abstraction Accumulate");
    summarize( temp1.c_str(), SIZE, iterations, kShowGMeans, kShowPenalty );


    // the sorting tests are much slower than the accumulation tests - O(N^2)
    iterations = iterations / 2000;
    
    // fill one set of random numbers
    fill_random( dMpb, dMpe );
    // copy to the other sets, so we have the same numbers
    ::copy( dMpb, dMpe, DVMpb );
    ::copy( dMpb, dMpe, DV10Mpb );

    test_insertion_sort(dMpb, dMpe, dpb, dpe, dZero, myTypeName + " insertion_sort pointer");
    test_insertion_sort(dMPb, dMPe, dPb, dPe, dZero, myTypeName + " insertion_sort pointer_class");
    test_insertion_sort(DVMpb, DVMpe, DVpb, DVpe, DVZero, myTypeName + " insertion_sort TypeValueWrapper pointer");
    test_insertion_sort(DVMPb, DVMPe, DVPb, DVPe, DVZero, myTypeName + " insertion_sort TypeValueWrapper pointer_class");
    test_insertion_sort(DV10Mpb, DV10Mpe, DV10pb, DV10pe, DV10Zero, myTypeName + " insertion_sort TypeValueWrapper10 pointer");
    test_insertion_sort(DV10MPb, DV10MPe, DV10Pb, DV10Pe, DV10Zero, myTypeName + " insertion_sort TypeValueWrapper10 pointer_class");
    test_insertion_sort(dRRMpb, dRRMpe, dRRpb, dRRpe, dZero, myTypeName + " insertion_sort reverse reverse pointer");
    test_insertion_sort(dRRMPb, dRRMPe, dRRPb, dRRPe, dZero, myTypeName + " insertion_sort reverse reverse pointer_class");
    test_insertion_sort(DVRRMPb, DVRRMPe, DVRRPb, DVRRPe, DVZero, myTypeName + " insertion_sort TypeValueWrapper reverse reverse pointer_class");

    std::string temp2( myTypeName + " Abstraction Insertion Sort");
    summarize( temp2.c_str(), SIZE, iterations, kShowGMeans, kShowPenalty );


    // these are slightly faster - O(NLog2(N))
    iterations = iterations * 8;

    test_quicksort(dMpb, dMpe, dpb, dpe, dZero, myTypeName + " quicksort pointer");
    test_quicksort(dMPb, dMPe, dPb, dPe, dZero, myTypeName + " quicksort pointer_class");
    test_quicksort(DVMpb, DVMpe, DVpb, DVpe, DVZero, myTypeName + " quicksort TypeValueWrapper pointer");
    test_quicksort(DVMPb, DVMPe, DVPb, DVPe, DVZero, myTypeName + " quicksort TypeValueWrapper pointer_class");
    test_quicksort(DV10Mpb, DV10Mpe, DV10pb, DV10pe, DV10Zero, myTypeName + " quicksort TypeValueWrapper10 pointer");
    test_quicksort(DV10MPb, DV10MPe, DV10Pb, DV10Pe, DV10Zero, myTypeName + " quicksort TypeValueWrapper10 pointer_class");
    test_quicksort(dRRMpb, dRRMpe, dRRpb, dRRpe, dZero, myTypeName + " quicksort reverse reverse pointer");
    test_quicksort(dRRMPb, dRRMPe, dRRPb, dRRPe, dZero, myTypeName + " quicksort reverse reverse pointer_class");
    test_quicksort(DVRRMPb, DVRRMPe, DVRRPb, DVRRPe, DVZero, myTypeName + " quicksort TypeValueWrapper reverse reverse pointer_class");

    std::string temp3( myTypeName + " Abstraction Quicksort");
    summarize( temp3.c_str(), SIZE, iterations, kShowGMeans, kShowPenalty );


    test_heap_sort(dMpb, dMpe, dpb, dpe, dZero, myTypeName + " heap_sort pointer");
    test_heap_sort(dMPb, dMPe, dPb, dPe, dZero, myTypeName + " heap_sort pointer_class");
    test_heap_sort(DVMpb, DVMpe, DVpb, DVpe, DVZero, myTypeName + " heap_sort TypeValueWrapper pointer");
    test_heap_sort(DVMPb, DVMPe, DVPb, DVPe, DVZero, myTypeName + " heap_sort TypeValueWrapper pointer_class");
    test_heap_sort(DV10Mpb, DV10Mpe, DV10pb, DV10pe, DV10Zero, myTypeName + " heap_sort TypeValueWrapper10 pointer");
    test_heap_sort(DV10MPb, DV10MPe, DV10Pb, DV10Pe, DV10Zero, myTypeName + " heap_sort TypeValueWrapper10 pointer_class");
    test_heap_sort(dRRMpb, dRRMpe, dRRpb, dRRpe, dZero, myTypeName + " heap_sort reverse reverse pointer");
    test_heap_sort(dRRMPb, dRRMPe, dRRPb, dRRPe, dZero, myTypeName + " heap_sort reverse reverse pointer_class");
    test_heap_sort(DVRRMPb, DVRRMPe, DVRRPb, DVRRPe, DVZero, myTypeName + " heap_sort TypeValueWrapper reverse reverse pointer_class");
    
    std::string temp4( myTypeName + " Abstraction Heap Sort");
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

#ifdef THESE_WORK_BUT_ARE_NOT_NEEDED_YET
    TestOneType<float>();
    TestOneType<long double>();     // this isn't well optimized overall yet (esp. insertion_sort and quick_sort)
#endif


    iterations *= 3;
    TestOneType<int32_t>();
    TestOneType<uint64_t>();

#ifdef THESE_WORK_BUT_ARE_NOT_NEEDED_YET
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
