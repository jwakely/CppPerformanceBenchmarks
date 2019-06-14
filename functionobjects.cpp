/*
    Copyright 2007-2008 Adobe Systems Incorporated
    Copyright 2018-2019 Chris Cox
    Distributed under the MIT License (see accompanying file LICENSE_1_0_0.txt
    or a copy at http://stlab.adobe.com/licenses.html )
    

    This test file started life as 
        ISO/IEC TR 18015:2006(E) Appendix D.4


Goals: Compare the performance of function pointers, functors, inline functors,
        lambdas, standard functors, and native comparison operators.

    Also compare the performance of qsort(), a simple qsort function, quicksort template, and std::sort.


Assumptions:
    
    1) Inline functors, standard functors and inlined native comparisons should perform similarly.
    
    2) Using functors should be faster than using function pointers.
    
    3) Inline functors should be as fast or faster than out of line functors.
    
    4) A template should be at least as fast as a hard coded function of the same algorithm, sometimes faster.
    
    5) std::sort should faster than the standard library function qsort().
    
    6) std::sort should be faster than a naive quicksort template using the same compare function.
 
    7) Lambdas should be as fast as an inline functor or native comparison.
        Currently they are not because compilers treat them as separate functions and call via function pointer.


Since qsort's comparison function must return int (less than 0, 0, greater than 0)
    and std::sort's must return a bool, it is not possible to test them with each
    other's comparator.

*/


/******************************************************************************/

#include "benchmark_stdint.hpp"
#include <functional>
#include <algorithm>
#include <cstdlib>
#include <memory>
#include "benchmark_results.h"
#include "benchmark_timer.h"
#include "benchmark_algorithms.h"

using namespace std;

/******************************************************************************/

template <class Iterator>
void verify_sorted(Iterator first, Iterator last, const char *label) {
    Iterator prev = first;
    first++;
    while (first != last) {
        if (*first++ < *prev++) {
            printf("test %s failed\n", label);
            break;
        }
    }
}

/******************************************************************************/
typedef int qless_func_type( const void *, const void *);

// qsort passes void * arguments to its comparison function,
// which must return negative, 0, or positive value
int
less_than_function1( const void * lhs, const void * rhs )
    {
    if( *(const double *) lhs < *(const double *) rhs ) return -1;
    if( *(const double *) lhs > *(const double *) rhs ) return 1;
    return 0;
    }

// std::sort, on the other hand, needs a comparator that returns true or false
typedef bool less_func_type( const double, const double );

bool
less_than_function2( const double lhs, const double rhs )
    {
    return( lhs < rhs? true : false );
    }

// the comparison operator in the following functor is defined out of line
struct less_than_functor
{
    bool operator()( const double& lhs, const double& rhs ) const;
};

bool
less_than_functor::operator()( const double& lhs, const double& rhs ) const
    {
    return( lhs < rhs? true : false );
    }

// the comparison operator in the following functor is defined inline
struct inline_less_than_functor
{
    inline bool operator()( const double& lhs, const double& rhs ) const
        {
        return( lhs < rhs? true : false );
        }
};

/******************************************************************************/

// comparison function passed in as a functor
template<class Iterator, typename Comparator>
void quicksort1(Iterator begin, Iterator end, Comparator compare)
{
    if ( (end - begin) > 1 ) {

        auto middleValue = *begin;
        Iterator left = begin;
        Iterator right = end;

        for(;;) {

            while ( compare( middleValue, *(--right) ) );
            if ( !(left < right ) ) break;
            while ( compare( *(left), middleValue ) )
                ++left;
            if ( !(left < right ) ) break;

            // swap
            auto temp = *right;
            *right = *left;
            *left = temp;
        }
        
        quicksort1( begin, right + 1, compare );
        quicksort1( right + 1, end, compare );
    }
}

/******************************************************************************/

typedef bool comparator_function( const double x, const double y );

// use a pointer to function as a template parameter
// exact function is known at compile time, and can be inlined
template<class Iterator, comparator_function compare>
void quicksort2(Iterator begin, Iterator end)
{
    if ( (end - begin) > 1 ) {

        auto middleValue = *begin;
        Iterator left = begin;
        Iterator right = end;

        for(;;) {

            while ( compare( middleValue, *(--right) ) );
            if ( !(left < right ) ) break;
            while ( compare( *(left), middleValue ) )
                ++left;
            if ( !(left < right ) ) break;

            // swap
            auto temp = *right;
            *right = *left;
            *left = temp;
        }
        
        quicksort2<Iterator,compare>( begin, right + 1 );
        quicksort2<Iterator,compare>( right + 1, end );
    }
}

/******************************************************************************/

// use a function pointer
// most compilers will not inline the function argument
void quicksort_function(double* begin, double* end, comparator_function compare)
{
    if ( (end - begin) > 1 ) {

        double middleValue = *begin;
        double* left = begin;
        double* right = end;

        for(;;) {

            while ( compare( middleValue, *(--right) ) );
            if ( !(left < right ) ) break;
            while ( compare( *(left), middleValue ) )
                ++left;
            if ( !(left < right ) ) break;

            // swap
            double temp = *right;
            *right = *left;
            *left = temp;
        }
        
        quicksort_function( begin, right + 1, compare );
        quicksort_function( right + 1, end, compare );
    }
}

/******************************************************************************/

inline
void swap_bytes( unsigned char *left, unsigned char *right, size_t size)
{
    switch(size) {
        case 2:
            {
            uint16_t tempS = *((uint16_t *)right);
            *((uint16_t *)right) = *((uint16_t *)left);
            *((uint16_t *)left) = tempS;
            }
            break;
        case 4:
            {
            uint32_t tempS = *((uint32_t *)right);
            *((uint32_t *)right) = *((uint32_t *)left);
            *((uint32_t *)left) = tempS;
            }
            break;
        case 8:
            {
            uint64_t tempS = *((uint64_t *)right);
            *((uint64_t *)right) = *((uint64_t *)left);
            *((uint64_t *)left) = tempS;
            }
            break;
        default:
            for (size_t j=0; j < size; ++j) {
                unsigned char temp = right[j];
                right[j] = left[j];
                left[j] = temp;
            }
            break;
    }
}

void qsort_inner(unsigned char *begin, unsigned char *end, size_t size, qless_func_type compare)
{
    while ( (end - begin) > size ) {
        unsigned char * left( begin );
        unsigned char * right( end );
        unsigned char * middle( begin );
        
        for(;;) {
            
            right -= size;
            while ( compare( middle, right ) < 0 )
                right -= size;
            if ( !(left < right ) ) break;
            while ( compare( left, middle ) < 0 )
                left += size;
            if ( !(left < right ) ) break;
            
            // swap pointer as needed to track middle value
            if (middle == left)
                middle = right;
            else if (middle == right)
                middle = left;
            swap_bytes(left,right,size);
        }
        
        // recurse first range
        qsort_inner( begin, (right+size), size, compare );
        
        // iterate last range instead of recursing
        begin = (right+size);   // end, size, compare unchanged
    }
    
}

// simple qsort implementation
void simple_qsort(void* start, size_t count, size_t size, qless_func_type compare)
{
    unsigned char *begin( static_cast<unsigned char *>(start) );
    unsigned char *end( begin + count * size );
    qsort_inner(begin,end,size,compare);
}

/******************************************************************************/
/******************************************************************************/

template < typename T, typename Sorter, typename Comparison >
void TestOneSort( const T master_table, T table,
                 const int tablesize, const int iterations,
                 Sorter doSort, Comparison doCompare,
                 const char *label)
{
    start_timer();
    for (int i = 0; i < iterations; ++i) {
        std::copy(master_table, master_table+tablesize, table);
        doSort( table, table+tablesize, doCompare );
        verify_sorted( table, table + tablesize, label );
    }
    record_result( timer(), label );
}

/******************************************************************************/

template < typename T, typename Sorter >
void TestOneSort( const T master_table, T table,
                 const int tablesize, const int iterations,
                 Sorter doSort,
                 const char *label)
{
    start_timer();
    for (int i = 0; i < iterations; ++i) {
        std::copy(master_table, master_table+tablesize, table);
        doSort( table, table+tablesize );
        verify_sorted( table, table + tablesize, label );
    }
    record_result( timer(), label );
}

/******************************************************************************/
/******************************************************************************/

// ugly, but some compilers can't infer types otherwise
template<typename T, typename Compare>
inline
void compare_sort(T a, T b, Compare c)
{
    std::sort(a,b,c);
}

/******************************************************************************/

// ugly, but some compilers can't infer types otherwise
template<typename T, comparator_function c>
inline
void compare_sort2(T a, T b)
{
    std::sort(a,b,c);
}

/******************************************************************************/

// ugly, but some compilers can't infer types otherwise
template<typename T>
inline
void plain_sort(T a, T b)
{
    std::sort(a,b);
}

/******************************************************************************/

// impedence matching
template<typename T>
inline
void qsort_func(T *a, T *b)
{
    qsort( a, (b-a), sizeof(T), less_than_function1 );
}

/******************************************************************************/

// impedence matching
template<typename T>
inline
void simple_qsort_func(T *a, T *b)
{
    simple_qsort( a, (b-a), sizeof(T), less_than_function1 );
}

/******************************************************************************/
/******************************************************************************/

int main(int argc, char* argv[])
{
    int iterations = (1 < argc) ? atoi(argv[1]) : 2000; // number of iterations
    int tablesize = (2 < argc) ? atoi(argv[2]) : 10000; // size of array
    
    
    // output command for documentation
    for (int i = 0; i < argc; ++i)
        printf("%s ", argv[i] );
    printf("\n");
    
    
    // seed the random number generator, so we get repeatable results
    scrand( tablesize + 123 );
    
    auto master_storage = std::unique_ptr<double>(new double[tablesize]);
    double * master_table = master_storage.get();
    for( int n = 0; n < tablesize; ++n )
        master_table[n] = static_cast<double>( crand32() );
    
    auto table_storage = std::unique_ptr<double>(new double[tablesize]);
    double * table = table_storage.get();

    
    TestOneSort( master_table, table, tablesize, iterations, qsort_func<double>,
                 "qsort array with function pointer" );
    
    TestOneSort( master_table, table, tablesize, iterations, simple_qsort_func<double>,
                "simple_qsort array with function pointer" );

    TestOneSort( master_table, table, tablesize, iterations, quicksort_function, less_than_function2,
                "quicksort function array with function pointer" );
    
    TestOneSort( master_table, table, tablesize, iterations, quicksort1<double *, less_func_type >, less_than_function2,
                "quicksort template array with function pointer" );
    
    // would prefer pure template, but some compilers can't infer types correctly
    TestOneSort( master_table, table, tablesize, iterations, compare_sort< double *, less_func_type >, less_than_function2,
                "std::sort array with function pointer" );
    
    TestOneSort( master_table, table, tablesize, iterations, quicksort2<double *, less_than_function2>,
                "quicksort template array with template function pointer" );
    
    TestOneSort( master_table, table, tablesize, iterations, compare_sort2< double *, less_than_function2 >,
                "std::sort array with template function pointer" );
    
    TestOneSort( master_table, table, tablesize, iterations, quicksort1<double *, less_than_functor>, less_than_functor(),
                "quicksort template array with user-supplied functor" );
    
    TestOneSort( master_table, table, tablesize, iterations, compare_sort< double *, less_than_functor >, less_than_functor(),
                "std::sort array with user-supplied functor" );
    
    TestOneSort( master_table, table, tablesize, iterations, quicksort1<double *, inline_less_than_functor >, inline_less_than_functor(),
                "quicksort template array with user-supplied inline functor" );
    
    TestOneSort( master_table, table, tablesize, iterations, compare_sort< double *, inline_less_than_functor >, inline_less_than_functor(),
                "std::sort array with user-supplied inline functor" );

// lambdas currently act like function pointers even though they should qualify for inlining
    TestOneSort( master_table, table, tablesize, iterations, quicksort1<double *, less_func_type >,
                [](double lhs, double rhs){ return( lhs < rhs? true : false ); },
                "quicksort template array with lambda function" );
    
    TestOneSort( master_table, table, tablesize, iterations, compare_sort< double *, less_func_type >,
                [](double lhs, double rhs){ return( lhs < rhs? true : false ); },
                "std::sort array with lambda function" );
    
    TestOneSort( master_table, table, tablesize, iterations, quicksort1<double *, less<double> >, less<double>(),
                "quicksort template array with standard functor" );
    
    TestOneSort( master_table, table, tablesize, iterations, compare_sort< double *, less<double> >, less<double>(),
                "std::sort array with standard functor" );
    
    TestOneSort( master_table, table, tablesize, iterations, quicksort<double *>,
                "quicksort template array with native < operator" );

    TestOneSort( master_table, table, tablesize, iterations, plain_sort<double *>,
                 "std::sort array with native < operator" );

    summarize("Function Objects", tablesize, iterations, kDontShowGMeans, kDontShowPenalty );

    
    return 0;
}
