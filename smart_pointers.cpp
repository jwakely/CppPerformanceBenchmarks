/*
    Copyright 2018-2019 Chris Cox
    Distributed under the MIT License (see accompanying file LICENSE_1_0_0.txt
    or a copy at http://stlab.adobe.com/licenses.html )


Goals: Compare the performance of various smart pointers and raw pointers.
        Creation, copying, dereferencing, testing NULL/non-NULL, sorting, freeing non-final, freeing final
        For small and medium sized objects.


Assumptions:

    1) Smart pointer allocation and deletion times should be minimized.
 
    2) Smart pointer dereference time should be close to the speed of a simple pointer.
 
    3) Smart pointer NULL/non-NULL test time should be close to the speed of a simple pointer.
 
    4) Shared pointers will take more time to create, copy, dereference, and delete. (due to reference count manipulation)
 
    5) Deletion of non-final shared and unique pointers should be very fast.


*/


/******************************************************************************/

#include "benchmark_stdint.hpp"
#include <functional>
#include <cstdlib>
#include <cstdio>
#include <cmath>
#include <memory>
#include <deque>
#include <vector>
#include <functional>
#include <algorithm>
#include "benchmark_timer.h"
#include "benchmark_typenames.h"

/******************************************************************************/

// some operations are fast, and can be iterated to get more accurate times/speed values
const int iterations = 20;


// auto_ptr tests might be of interest to people still using auto_ptr.
//  Safe with llvm and (_LIBCPP_STD_VER <= 14)
//  But unsafe on other compilers.
//#define ENABLE_AUTOPTR true

/******************************************************************************/

// to keep math comparible between sizes, restrict comparisons to first value
template<typename T, int N>
struct VariableContainer {
    T values[N];
    
    VariableContainer(T x) { for (int i = 0; i < N; ++i) { values[i] = x; } }
    
    T first() const { return values[0]; }
    
    template<typename TT>
    inline operator TT () const { return (TT)values[0]; }
};

template<typename T, int N>
inline bool operator<(const VariableContainer<T,N>& x, const VariableContainer<T,N>& y) {
    return ( x.values[0] < y.values[0] );
}

template<typename T, int N>
inline bool operator<=(const VariableContainer<T,N>& x, const VariableContainer<T,N>& y) {
    return ( x.values[0] <= y.values[0] );
}

template<typename T, int N>
inline bool operator!=(const VariableContainer<T,N>& x, const VariableContainer<T,N>& y) {
    return ( x.values[0] != y.values[0] );
}

template<typename T, int N>
inline bool operator==(const VariableContainer<T,N>& x, const VariableContainer<T,N>& y) {
    return ( x.values[0] == y.values[0] );
}

/******************************************************************************/

// a pointer wrapped in a struct, aka an iterator
// more or less a primitive unique_ptr
template<typename T>
struct PointerWrapper {
    T* current;
    PointerWrapper() : current(nullptr) {}
    PointerWrapper(T* x) : current(x) {}
    T& operator*() const { return *current; }
    T* get() const { return current; }
    
    void reset( T* x = nullptr ) {
        delete current;
        current = x;
    }
    
    typedef T * PointerWrapper::*unspecified_bool_type;
    
    operator unspecified_bool_type() const
    {
        return (current == nullptr) ? 0 : &PointerWrapper::current;
    }
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
/******************************************************************************/

template<class Iterator>
Iterator medianOfThreeDeref( Iterator a, Iterator b, Iterator c )
{
     if (**b < **a)
         std::swap(*a,*b);
     if (**c < **a)
         std::swap(*a,*c);
     if (**c < **b)
         std::swap(*b,*c);
     return b;
}

// This means everyone will be tested with the same code (while std::sort gets the local flavor)
template<class Iterator>
void quicksort_deref(Iterator begin, Iterator end)
{
    while ( (end - begin) > 1 ) {

        // does ok, but not great, with duplicate values
        auto dist = (end - begin);
        auto mm = begin + (dist-1)/2;
        auto ee = end - 1;
        auto middleValue = ** medianOfThreeDeref( begin, mm, ee );
        
        Iterator left = begin;
        Iterator right = end;
        
        for(;;) {
            while ( !( (**(--right)) < middleValue ) && (right > left) );
            if ( !(left < right ) ) break;
            
            while ( (left < right)  && !( middleValue < (**(left)) ) )
                ++left;
            if ( !(left < right ) ) break;
            
            // swap the smart pointers
            std::swap(*right,*left);    // faster as std::swap(**right,**left);  but swapping pointer containers is what we're testing
        }
        
        // recurse smaller range, iterate on larger range
        if ( (end-right) < (right-begin) ) {
            quicksort_deref( right+1, end );
            end = right + 1;
        } else {
            quicksort_deref( begin, right+1 );
            begin = right + 1;
        }
    }
}

/******************************************************************************/
/******************************************************************************/

template< typename TT >
void validate_sum(TT sum, size_t tablesize, int N, int init_value, const std::string label)
{
    TT temp = TT(tablesize * init_value);
    if (sum != temp)
        printf("test %s failed\n", label.c_str());
}

/******************************************************************************/

void validate_nulls(int count, size_t tablesize, const std::string label)
{
    if (count != tablesize)
        printf("test %s failed\n", label.c_str());
}

/******************************************************************************/

template <class Iterator>
void verify_sorted(Iterator first, Iterator last, const std::string label) {
    Iterator current = first;
    Iterator previous = first;
    current++;
    while (current != last) {
        if (**(current++) < **(previous++)) {
            printf("test %s failed\n", label.c_str());
            break;
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

void record_result( double time, size_t count, size_t iterations, std::string label ) {
    gResults.push_back( one_result(time,count,iterations,label) );
}

/******************************************************************************/

void summarize(std::string name) {
    size_t i;
    double total_absolute_times = 0.0;
    
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
    
    printf("\ntest %*s description    absolute   operations\n", (int)longest_label_len-12, " ");
    printf("number %*s  time       per second\n\n", (int)longest_label_len, " ");
    
    for (i = 0; i < current_test; ++i) {
        const double timeThreshold = 1.0e-4;
        
        // calculate total time
        total_absolute_times += gResults[i].time;
        
        double millions = ((double)(gResults[i].count) * gResults[i].iterations)/1.0e6;
        
        double speed = 1.0;
        if (gResults[i].time < timeThreshold) {
            speed = INFINITY;
        } else
            speed = millions/gResults[i].time;
        
        printf("%3i %*s\"%s\"  %5.2f sec   %5.5g M\n",
               (int)i,
               (int)(longest_label_len - gResults[i].label.size()),
               "",
               gResults[i].label.c_str(),
               gResults[i].time,
               speed);
    }
    
    // report total time
    printf("\nTotal absolute time for %s: %.2f sec\n", name.c_str(), total_absolute_times);
    
    // reset the test counter so we can run more tests
    gResults.clear();
}

/******************************************************************************/
/******************************************************************************/

// sigh, can't simplify this til C++14 is better supported
template < typename T, typename PC >
std::vector< PC >
CreateArray(int tablesize, int value, const std::string label)
{
    // This block allocation shows excessive variation (OS/malloc state) from run to run.
    // So move it outside the timer.
    std::vector< PC > result;
    result.resize(tablesize);

    start_timer();
    
    for (int i = 0; i < tablesize; ++i) {
        result[i] = PC( new T(value) );
    }
    
    auto time = timer();
    record_result( time, 1, tablesize, label );
    return result;
}

/******************************************************************************/

template < typename T >
std::vector< T >
CopyArray( std::vector< T > &input, const std::string label)
{
    size_t tablesize( input.size() );
    
    // This block allocation shows excessive variation (OS/malloc state) from run to run.
    // So move it outside the timer.
    std::vector< T > result;
    result.resize(tablesize);
    
    start_timer();
    
    for (size_t i = 0; i < tablesize; ++i) {
        result[i] = input[i];
    }
    
    auto time = timer();
    record_result( time, 1, tablesize, label );
    return result;
}

/******************************************************************************/

// unique_ptr cannot copy, just move
template < typename T >
std::vector< std::unique_ptr<T> >
CopyArray( std::vector< std::unique_ptr<T> > &input, const std::string label)
{
    size_t tablesize( input.size() );
    
    // This block allocation shows excessive variation (OS/malloc state) from run to run.
    // So move it outside the timer.
    std::vector< std::unique_ptr<T> > result;
    result.resize(tablesize);
    
    start_timer();
    
    for (size_t i = 0; i < tablesize; ++i) {
        result[i] = std::move( input[i] );
    }
    
    auto time = timer();
    record_result( time, 1, tablesize, label );
    return result;
}

/******************************************************************************/

#if ENABLE_AUTOPTR

// auto_ptr lacks a NULL comparison, have to use get
template < typename T >
void TestNullArray( std::vector< std::auto_ptr<T> > &array, const std::string label)
{
    size_t tablesize( array.size() );
    int count = 0;
    
    start_timer();
    for (size_t i = 0; i < tablesize; ++i) {
        if (array[i].get()) count++;
    }
    
    auto time = timer();
    record_result( time, 1, tablesize, label );
    
    validate_nulls(count,tablesize,label);
}
#endif

/******************************************************************************/

template < typename T >
void TestNullArray( std::vector< T > &array, const std::string label)
{
    size_t tablesize( array.size() );
    int count;
    
    start_timer();
    for (size_t k = 0; k < iterations; ++k) {
        count = 0;
        for (size_t i = 0; i < tablesize; ++i) {
            if (array[i]) count++;              // this works for all but auto_ptr
            //count += (array[i] != nullptr);   // also works for all but auto_ptr
        }
    }
    
    auto time = timer();
    record_result( time, iterations, tablesize, label );
    
    validate_nulls(count,tablesize,label);
}

/******************************************************************************/

// use first element only, so speeds are comparable at different sizes
template < typename T >
void DereferenceSumArray( std::vector< T > &array, int N, int init_value, const std::string label)
{
    size_t tablesize( array.size() );
    auto sum = (*(array[0])).first();
    
    start_timer();
    for (size_t k = 0; k < iterations; ++k) {
        sum = (*(array[0])).first();
        for (size_t i = 1; i < tablesize; ++i) {
            sum += (*(array[i])).first();
        }
    }
    
    auto time = timer();
    record_result( time, iterations, tablesize, label );
    
    validate_sum(sum,tablesize,N,init_value,label);
}

/******************************************************************************/

// reproduce the same values, but without timing
template < typename T >
void JustRandomValuesArray( std::vector< T > &array)
{
    // make the values repeatable
    srand( 123 );
    for (auto iter = array.begin(); iter != array.end(); ++iter)
        **iter = ((rand()>>5) & 65535);
}

/******************************************************************************/

template < typename T >
void RandomValuesArray( std::vector< T > &array, const std::string label)
{
    // make the values repeatable
    srand( 123 );
    
    start_timer();
    
    for (size_t k = 0; k < iterations; ++k) {
        for (auto iter = array.begin(); iter != array.end(); ++iter)
            **iter = ((rand()>>5) & 65535);
    }
    
    auto time = timer();
    record_result( time, iterations, array.size(), label );
}

/******************************************************************************/

template < typename T >
void QuickSortArray( std::vector< T > &array, const std::string label)
{
    // uses random values from previous step!
    
    start_timer();
    quicksort_deref( array.begin(), array.end() );
    
    auto time = timer();
    record_result( time, 1, array.size(), label );
    
    verify_sorted(array.begin(),array.end(), label);
}

/******************************************************************************/

template < typename T >
void StdSortArray( std::vector< T > &array, const std::string label)
{
    // uses random values from previous step!
    
    start_timer();
    std::sort( array.begin(), array.end(), [](T &lhs, T &rhs){ return( (*lhs < *rhs) ? true : false ); } );
    
    auto time = timer();
    record_result( time, 1, array.size(), label );
    
    verify_sorted(array.begin(),array.end(), label);
}

/******************************************************************************/

template < typename T >
void FreeArray( std::vector< T > &array, const std::string label)
{
    size_t tablesize( array.size() );
    
    start_timer();
    for (size_t i = 0; i < tablesize; ++i) {
        array[i].reset();
    }
    
    auto time = timer();
    record_result( time, 1, tablesize, label );
}

/******************************************************************************/

template < typename T >
void FreeArray( std::vector< T* > &array, const std::string label)
{
    size_t tablesize( array.size() );
    
    start_timer();
    for (size_t i = 0; i < tablesize; ++i) {
        delete array[i];
    }
    
    auto time = timer();
    record_result( time, 1, tablesize, label );
}

/******************************************************************************/
/******************************************************************************/

template< typename T, typename TT>
void TestOnePointer( int tablesize, int init_value, int N, bool can_free_nonfinal, std::string label)
{
    auto test_array = CreateArray<T, TT>( tablesize, init_value, label + " create array" );
    auto new_array = CopyArray( test_array, label + " copy array" );
    DereferenceSumArray( new_array, N, init_value, label + " dereference array" );
    TestNullArray( new_array, label + " test null array" );
    
    // this replaces values in the objects
    RandomValuesArray( new_array, label + " random_values");
    QuickSortArray( new_array, label + " quicksort array");
    
    JustRandomValuesArray( new_array );
    StdSortArray( new_array, label + " std::sort array");
    
    // not all types can free the non-final array
    if (can_free_nonfinal)
        FreeArray( test_array, label + " delete array non-final" );
    FreeArray( new_array, label + " delete array final" );

    summarize( label + " Smart Pointers" );
}

/******************************************************************************/

template<typename T, int N>
void TestOneType( int tablesize, int init_value )
{
    std::string myTypeName( getTypeName<T>() + "X" + std::to_string(N) );
    
    using vctN = VariableContainer<T,N>;

    using ppVCTN = vctN *;
    TestOnePointer<vctN, ppVCTN>( tablesize, init_value, N, false, myTypeName + " pointer");

    using pwVCTN = PointerWrapper< vctN >;
    TestOnePointer<vctN, pwVCTN>( tablesize, init_value, N, false, myTypeName + " wrapped_ptr");

    using upVCTN = std::unique_ptr< vctN >;
    TestOnePointer<vctN, upVCTN>( tablesize, init_value, N, true, myTypeName + " unique_ptr");

#if ENABLE_AUTOPTR
    using apVCTN = std::auto_ptr< vctN >;
    TestOnePointer<vctN, apVCTN>( tablesize, init_value, N, true, myTypeName + " auto_ptr");
#endif

    using spVCTN = std::shared_ptr< vctN >;
    TestOnePointer<vctN, spVCTN>( tablesize, init_value, N, true, myTypeName + " shared_ptr");
}

/******************************************************************************/
/******************************************************************************/

int main(int argc, char* argv[])
{
    int init_value = (1 < argc) ? atoi(argv[1]) : 1;    // init value
    int tablesize = (2 < argc) ? atoi(argv[2]) : 5000000; // size of array
    
    // output command for documentation
    for (int i = 0; i < argc; ++i)
        printf("%s ", argv[i] );
    printf("\n");

    
    TestOneType<int64_t,1>( tablesize, init_value );
    TestOneType<int64_t,13>( tablesize, init_value );


#if TESTED_WORKS_UNUSED
    // types above show the interesting behavior, so far.
    TestOneType<int16_t,1>( tablesize, init_value );
    TestOneType<uint16_t,1>( tablesize, init_value );
    TestOneType<int32_t,1>( tablesize, init_value );
    TestOneType<uint32_t,1>( tablesize, init_value );
    TestOneType<int64_t,1>( tablesize, init_value );
    TestOneType<uint64_t,1>( tablesize, init_value );
    TestOneType<float,1>( tablesize, init_value );
    TestOneType<double,1>( tablesize, init_value );
#endif


#if TESTED_WORKS_BUT_SLOW
    // The naive quicksort shows bad behavior when there are too many repeated values.
    // And truncation of values into 8 bits creates a lot of repeats.
    TestOneType<uint8_t,1>( tablesize, init_value );
    TestOneType<int8_t,1>( tablesize, init_value );
#endif


    return 0;
}
