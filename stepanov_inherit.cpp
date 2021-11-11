/*
    Copyright 2007-2009 Adobe Systems Incorporated
    Copyright 2018-2019 Chris Cox
    Distributed under the MIT License (see accompanying file LICENSE_1_0_0.txt
    or a copy at http://stlab.adobe.com/licenses.html )


Goal:  Examine any change in performance when using class inheritance.


Assumptions:
    
    1) Child classes with no virtual methods will perform no worse than parent classes or childless classes.
        Regardless of depth of subclassing.

    2) Child classes with virtual methods, where the object type is exactly known at compile time,
        will perform no worse than parent classes or childless classes.
        The compiler should recognize monomorphic use of polymorphic objects and optimize away virtual dispatch.
    
    3) Methods declared virtual, but with no overrides, will perform no worse than non-virtual methods.
        The compiler should recognize these methods and treat them as non-virtual.


History:
    This is similar to Alex Stepanov's original abstraction penalty benchmark.
    
    But the use of inheritance complicates the benchmark enough that it was broken out
    into a separate test.


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
/******************************************************************************/

// this constant may need to be adjusted to give reasonable minimum times
// For best results, times should be about 1.0 seconds for the minimum test run
int iterations = 2100000;

// 2000 items, or about 16k of data
// this is intended to remain within the L2 cache of most common CPUs
const int SIZE = 2000;

// initial value for filling our arrays, may be changed from the command line
double init_value = 3.0;

/******************************************************************************/
/******************************************************************************/

// A single value wrapped in a class
template<typename T>
struct TypeClass {
    T value;
    TypeClass() {}
    TypeClass(const T& x) : value(x) {}
    operator T() { return value; }
};

template<typename T>
inline TypeClass<T> operator+(const TypeClass<T>& x, const TypeClass<T>& y) {
    return TypeClass<T>(x.value + y.value);
}

/******************************************************************************/

// A subclass of TypeClass, with no changes that interfere with the parent class.
template<typename T>
struct TypeSubClass : public TypeClass<T> {
    TypeSubClass() {}
    TypeSubClass(const T& x) : TypeClass<T>(x) {}

    // this function is unused in this test - this is just to add something beyond the parent class
    inline TypeSubClass do_nothing (const TypeSubClass& x, const TypeSubClass& y) {
        return TypeSubClass<T>(x.value * y.value);
    }
};

template<typename T>
inline TypeSubClass<T> operator+(const TypeSubClass<T>& x, const TypeSubClass<T>& y) {
    return TypeSubClass<T>(x.value + y.value);
}

/******************************************************************************/

// A subclass of TypeSubClass, with no changes that interfere with the parent class.
template<typename T>
struct TypeSub2Class : public TypeSubClass<T> {
    TypeSub2Class() {}
    TypeSub2Class(const T& x) : TypeSubClass<T>(x) {}

    // this function is unused in this test - this is just to add something beyond the parent class
    inline TypeSub2Class do_nothing2 (const TypeSub2Class& x, const TypeSub2Class& y) {
        return TypeSub2Class<T>(x.value * y.value);
    }
};

template<typename T>
inline TypeSub2Class<T> operator+(const TypeSub2Class<T>& x, const TypeSub2Class<T>& y) {
    return TypeSub2Class<T>(x.value + y.value);
}

/******************************************************************************/

// a "do nothing" parent class
template<typename T>
struct EmptyClass {
    EmptyClass() {}
    EmptyClass(const T& x) {}
};

// a subclass that does nothing
template<typename T>
struct EmptyClass1 : public EmptyClass<T> {
    EmptyClass1() {}
    EmptyClass1(const T& x) : EmptyClass<T>(x) {}
};

// a subclass that does nothing
template<typename T>
struct EmptyClass2 : public EmptyClass1<T> {
    EmptyClass2() {}
    EmptyClass2(const T& x) : EmptyClass1<T>(x) {}
};

// a subclass that does nothing
template<typename T>
struct EmptyClass3 : public EmptyClass2<T> {
    EmptyClass3() {}
    EmptyClass3(const T& x) : EmptyClass2<T>(x) {}
};

// a subclass that does nothing
template<typename T>
struct EmptyClass4 : public EmptyClass3<T> {
    EmptyClass4() {}
    EmptyClass4(const T& x) : EmptyClass3<T>(x) {}
};

// A child class that holds data and does the work.
// Exact type usage is known at compile time.
// Accessor is monomorphic.
template<typename T>
struct DeepSubClass : public EmptyClass4<T> {
    T value;
    DeepSubClass() {}
    DeepSubClass(const T& x) : EmptyClass4<T>(x), value(x) {}
    operator T() { return value; }
};

template<typename T>
inline DeepSubClass<T> operator+(const DeepSubClass<T>& x, const DeepSubClass<T>& y) {
    return DeepSubClass<T>(x.value + y.value);
}

/******************************************************************************/

// A single value wrapped in a class, with a virtual accessor.
// There is no subclass, so the exact type is known at compile time.
// There is no override, so the accessor is monomorphic, no VTable access needed.
template<typename T>
struct TypeVirtualClass {
    T value;
    TypeVirtualClass() {}
    TypeVirtualClass(const T& x) : value(x) {}
    virtual operator T() { return value; }
};

template<typename T>
inline TypeVirtualClass<T> operator+(const TypeVirtualClass<T>& x, const TypeVirtualClass<T>& y) {
    return TypeVirtualClass<T>(x.value + y.value);
}

/******************************************************************************/

// A single value wrapped in a class, with a virtual accessor
template<typename T>
struct TypeVirtualParentClass {
    T value;
    TypeVirtualParentClass() {}
    TypeVirtualParentClass(const T& x) : value(x) {}

    // this accessor will be unused
    virtual operator T() { return T(-1.0); }
};

// A subclass of TypeVirtualParentClass.
// Exact type usage is known at compile time.
// Accessor is polymorphic, but can be resolved exactly, no VTable access needed.
template<typename T>
struct TypeVirtualSubClass : public TypeVirtualParentClass<T> {
    typedef TypeVirtualParentClass<T> _parent;
    TypeVirtualSubClass() {}
    TypeVirtualSubClass(const T& x) : TypeVirtualParentClass<T>(x) {}

    // only this accessor can be used
    virtual operator T() { return _parent::value; }

    // this function is unused in this test - this is just to add something beyond the parent class
    virtual TypeVirtualSubClass<T> do_mult (const TypeVirtualSubClass<T>& x, const TypeVirtualSubClass<T>& y) {
        return TypeVirtualSubClass<T>(x.value * y.value);
    }
};

template<typename T>
inline TypeVirtualSubClass<T> operator+(const TypeVirtualSubClass<T>& x, const TypeVirtualSubClass<T>& y) {
    return TypeVirtualSubClass<T>(x.value + y.value);
}

/******************************************************************************/

// a "do nothing" parent class with a virtual accessor
template<typename T>
struct InterfaceClass {
    InterfaceClass() {}
    InterfaceClass(const T& x) {}
    virtual operator T() { return T(0.0); }
};

template<typename T>
inline InterfaceClass<T> operator+(const InterfaceClass<T>& x, const InterfaceClass<T>& y) {
    return InterfaceClass<T>(2.0);
}

// A child class that holds data and does the work.
// Exact type usage is known at compile time.
// Accessor is polymorphic, but can be resolved exactly, no VTable access needed.
template<typename T>
struct WorkerSubClass : public InterfaceClass<T> {
    T value;
    WorkerSubClass() {}
    WorkerSubClass(const T& x) : InterfaceClass<T>(x), value(x) {}
    virtual operator T() { return T(value); }
};

template<typename T>
inline WorkerSubClass<T> operator+(const WorkerSubClass<T>& x, const WorkerSubClass<T>& y) {
    return WorkerSubClass<T>(x.value + y.value);
}

/******************************************************************************/

// a "do nothing" parent class with a virtual accessor
template<typename T>
struct InterfaceClassB {
    InterfaceClassB() {}
    InterfaceClassB(const T& x) {}
    virtual operator T() { return T(0.0); }
};

template<typename T>
inline InterfaceClassB<T> operator+(const InterfaceClassB<T>& x, const InterfaceClassB<T>& y) {
    return InterfaceClassB<T>(2.0);
}

// a subclass that does nothing
template<typename T>
struct InterfaceClass1 : public InterfaceClassB<T> {
    InterfaceClass1() {}
    InterfaceClass1(const T& x) :InterfaceClassB<T>(x) {}
};

// a subclass that does nothing
template<typename T>
struct InterfaceClass2 : public InterfaceClass1<T> {
    InterfaceClass2() {}
    InterfaceClass2(const T& x) : InterfaceClass1<T>(x) {}
    virtual operator T() { return T(99.0); }
};

// a subclass that does nothing
template<typename T>
struct InterfaceClass3 : public InterfaceClass2<T> {
    InterfaceClass3() {}
    InterfaceClass3(const T& x) : InterfaceClass2<T>(x) {}
};

// a subclass that does nothing
template<typename T>
struct InterfaceClass4 : public InterfaceClass3<T> {
    InterfaceClass4() {}
    InterfaceClass4(const T& x) : InterfaceClass3<T>(x) {}
    virtual operator T() { return T(97.0); }
};

// A child class that holds data and does the work.
// Exact type usage is known at compile time.
// Accessor is polymorphic, but can be resolved exactly, no VTable access needed.
template<typename T>
struct WorkerDeepSubClass : public InterfaceClass4<T> {
    T value;
    WorkerDeepSubClass() {}
    WorkerDeepSubClass(const T& x) : InterfaceClass4<T>(x), value(x) {}
    virtual operator T() { return value; }
};

template<typename T>
inline WorkerDeepSubClass<T> operator+(const WorkerDeepSubClass<T>& x, const WorkerDeepSubClass<T>& y) {
    return WorkerDeepSubClass<T>(x.value + y.value);
}

/******************************************************************************/

// a "do nothing" abstract parent class with a virtual accessor
template<typename T>
struct InterfaceClassA {
    InterfaceClassA() {}
    InterfaceClassA(const T& x) {}
    virtual operator T() = 0;  // explicitly state that this cannot be called
};

// A child class that holds data and does the work.
// Exact type usage is known at compile time.
// Accessor is polymorphic, but can be resolved exactly, no VTable access needed.
template<typename T>
struct WorkerSubClass2 : public InterfaceClassA<T> {
    T value;
    WorkerSubClass2() {}
    WorkerSubClass2(const T& x) : InterfaceClassA<T>(x), value(x) {}
    virtual operator T() { return value; }
};

template<typename T>
inline WorkerSubClass2<T> operator+(const WorkerSubClass2<T>& x, const WorkerSubClass2<T>& y) {
    return WorkerSubClass2<T>(x.value + y.value);
}

/******************************************************************************/

// a "do nothing" abstract parent class with a virtual accessor
template<typename T>
struct InterfaceClassC {
    InterfaceClassC() {}
    InterfaceClassC(const T& x) {}
    virtual operator T() { return T(0.0); }
};

// A child class that holds data and does the work.
// Exact type usage is known at compile time.
// Accessor is polymorphic (with siblings), but can be resolved exactly, no VTable access needed.
template<typename T>
struct WorkerSubClass3 : public InterfaceClassC<T> {
    T value;
    WorkerSubClass3() {}
    WorkerSubClass3(const T& x) : InterfaceClassC<T>(x), value(x) {}
    virtual int do_nothing3() { return 3; }
    virtual operator T() { return value; }
};

template<typename T>
inline WorkerSubClass3<T> operator+(const WorkerSubClass3<T>& x, const WorkerSubClass3<T>& y) {
    return WorkerSubClass3<T>(x.value + y.value);
}

// A child class that holds data and does the work.
// Exact type usage is known at compile time.
// Accessor is polymorphic (with siblings), but can be resolved exactly, no VTable access needed.
template<typename T>
struct WorkerSubClass4 : public InterfaceClassC<T> {
    T value;
    WorkerSubClass4() {}
    WorkerSubClass4(const T& x) : InterfaceClassC<T>(x), value(x) {}
    virtual double do_nothing4() { return 4.0; }
    virtual operator T() { return value; }
};

template<typename T>
inline WorkerSubClass4<T> operator+(const WorkerSubClass4<T>& x, const WorkerSubClass4<T>& y) {
    return WorkerSubClass4<T>(x.value + y.value);
}

// A child class that holds data and does the work.
// Exact type usage is known at compile time.
// Accessor is polymorphic (with siblings), but can be resolved exactly, no VTable access needed.
template<typename T>
struct WorkerSubClass5 : public InterfaceClassC<T> {
    T value;
    WorkerSubClass5() {}
    WorkerSubClass5(const T& x) : InterfaceClassC<T>(x), value(x) {}
    virtual void do_nothing5() {}
    virtual operator T() { return value; }
};

template<typename T>
inline WorkerSubClass5<T> operator+(const WorkerSubClass5<T>& x, const WorkerSubClass5<T>& y) {
    return WorkerSubClass5<T>(x.value + y.value);
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

template <typename Iterator, typename T>
void test_accumulate(Iterator first, Iterator last, T zero, const std::string label) {
  
  start_timer();
  
  for(int i = 0; i < iterations; ++i)
    check_sum( T( accumulate(first, last, zero) ), label );
    
  record_std_result( timer(), label );
}

/******************************************************************************/

template <typename Iterator, typename T>
void test_insertion_sort(Iterator firstSource, Iterator lastSource, Iterator firstDest,
                        Iterator lastDest, T zero, const std::string label) {

    start_timer();

    for(int i = 0; i < iterations; ++i) {
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

    start_timer();

    for(int i = 0; i < iterations; ++i) {
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

    start_timer();

    for(int i = 0; i < iterations; ++i) {
        ::copy(firstSource, lastSource, firstDest);
        heapsort( firstDest, lastDest );
        verify_sorted( firstDest, lastDest, label );
    }
    
    record_std_result( timer(), label );
}

/******************************************************************************/
/******************************************************************************/


template<typename T>
void TestOneType()
{
    // our arrays of numbers to be summed
    T data[SIZE];
    TypeClass<T> Data[SIZE];
    TypeSubClass<T> DSData[SIZE];
    TypeSub2Class<T> DS2Data[SIZE];
    DeepSubClass<T> DeepData[SIZE];
    TypeVirtualClass<T> DVData[SIZE];
    TypeVirtualSubClass<T> DVSData[SIZE];
    WorkerSubClass<T> WSData[SIZE];
    WorkerDeepSubClass<T> WDData[SIZE];
    WorkerSubClass2<T> WSData2[SIZE];
    WorkerSubClass3<T> WSData3[SIZE];

    T dataMaster[SIZE];
    TypeClass<T> DataMaster[SIZE];
    TypeSubClass<T> DSDataMaster[SIZE];
    TypeSub2Class<T> DS2DataMaster[SIZE];
    DeepSubClass<T> DeepDataMaster[SIZE];
    TypeVirtualClass<T> DVDataMaster[SIZE];
    TypeVirtualSubClass<T> DVSDataMaster[SIZE];
    WorkerSubClass<T> WSDataMaster[SIZE];
    WorkerDeepSubClass<T> WDDataMaster[SIZE];
    WorkerSubClass2<T> WSDataMaster2[SIZE];
    WorkerSubClass3<T> WSDataMaster3[SIZE];

    // declaration of our iterator types and begin/end pairs
    typedef T* dp;
    dp dpb = data;
    dp dpe = data + SIZE;
    dp dMpb = dataMaster;
    dp dMpe = dataMaster + SIZE;

    typedef TypeClass<T> * Dp;
    Dp Dpb = Data;
    Dp Dpe = Data + SIZE;
    Dp DMpb = DataMaster;
    Dp DMpe = DataMaster + SIZE;

    typedef TypeSubClass<T> * DSp;
    DSp DSpb = DSData;
    DSp DSpe = DSData + SIZE;
    DSp DSMpb = DSDataMaster;
    DSp DSMpe = DSDataMaster + SIZE;

    typedef TypeSub2Class<T> * DS2p;
    DS2p DS2pb = DS2Data;
    DS2p DS2pe = DS2Data + SIZE;
    DS2p DS2Mpb = DS2DataMaster;
    DS2p DS2Mpe = DS2DataMaster + SIZE;

    typedef DeepSubClass<T> * DDp;
    DDp DDpb = DeepData;
    DDp DDpe = DeepData + SIZE;
    DDp DDMpb = DeepDataMaster;
    DDp DDMpe = DeepDataMaster + SIZE;

    typedef TypeVirtualClass<T> * DVp;
    DVp DVpb = DVData;
    DVp DVpe = DVData + SIZE;
    DVp DVMpb = DVDataMaster;
    DVp DVMpe = DVDataMaster + SIZE;

    typedef TypeVirtualSubClass<T> * DVSp;
    DVSp DVSpb = DVSData;
    DVSp DVSpe = DVSData + SIZE;
    DVSp DVSMpb = DVSDataMaster;
    DVSp DVSMpe = DVSDataMaster + SIZE;

    typedef WorkerSubClass<T> * DWSp;
    DWSp DWSpb = WSData;
    DWSp DWSpe = WSData + SIZE;
    DWSp DWSMpb = WSDataMaster;
    DWSp DWSMpe = WSDataMaster + SIZE;

    typedef WorkerDeepSubClass<T> * DWDp;
    DWDp DWDpb = WDData;
    DWDp DWDpe = WDData + SIZE;
    DWDp DWDMpb = WDDataMaster;
    DWDp DWDMpe = WDDataMaster + SIZE;

    typedef WorkerSubClass2<T> * DWS2p;
    DWS2p DWS2pb = WSData2;
    DWS2p DWS2pe = WSData2 + SIZE;
    DWS2p DWSM2pb = WSDataMaster2;
    DWS2p DWSM2pe = WSDataMaster2 + SIZE;

    typedef WorkerSubClass3<T> * DWS3p;
    DWS3p DWS3pb = WSData3;
    DWS3p DWS3pe = WSData3 + SIZE;
    DWS3p DWSM3pb = WSDataMaster3;
    DWS3p DWSM3pe = WSDataMaster3 + SIZE;


    T dZero = 0.0;
    TypeClass<T> DZero = 0.0;
    
    
    std::string myTypeName( getTypeName<T>() );
    
    gLabels.clear();
    
    int base_iterations = iterations;


    // seed the random number generator so we get repeatable results
    scrand( (int)init_value + 345 );

    fill( dpb, dpe, T(init_value) );
    fill( Dpb, Dpe, TypeClass<T>(init_value) );
    fill( DSpb, DSpe, TypeSubClass<T>(init_value) );
    fill( DS2pb, DS2pe, TypeSub2Class<T>(init_value) );
    fill( DVpb, DVpe, TypeVirtualClass<T>(init_value) );
    fill( DVSpb, DVSpe, TypeVirtualSubClass<T>(init_value) );
    fill( DWSpb, DWSpe, WorkerSubClass<T>(init_value) );
    fill( DWDpb, DWDpe, WorkerDeepSubClass<T>(init_value) );
    fill( DDpb, DDpe, DeepSubClass<T>(init_value) );
    fill( DWS2pb, DWS2pe, WorkerSubClass2<T>(init_value) );
    fill( DWS3pb, DWS3pe, WorkerSubClass3<T>(init_value) );

    test_accumulate( dpb, dpe, dZero, myTypeName + " accumulate pointer verify1");
    test_accumulate( Dpb, Dpe, TypeClass<T>(dZero), myTypeName + " accumulate TypeClass pointer verify1");
    test_accumulate( DSpb, DSpe, TypeSubClass<T>(dZero), myTypeName + " accumulate TypeSubClass pointer");
    test_accumulate( DS2pb, DS2pe, TypeSub2Class<T>(dZero), myTypeName + " accumulate TypeSub2Class pointer");
    test_accumulate( DDpb, DDpe, DeepSubClass<T>(dZero), myTypeName + " accumulate DeepSubClass pointer");
    test_accumulate( DVpb, DVpe, TypeVirtualClass<T>(dZero), myTypeName + " accumulate TypeVirtualClass pointer");
    test_accumulate( DVSpb, DVSpe, TypeVirtualSubClass<T>(dZero), myTypeName + " accumulate TypeVirtualSubClass pointer");
    test_accumulate( DWSpb, DWSpe, WorkerSubClass<T>(dZero), myTypeName + " accumulate WorkerSubClass pointer");
    test_accumulate( DWDpb, DWDpe, WorkerDeepSubClass<T>(dZero), myTypeName + " accumulate WorkerDeepSubClass pointer");
    test_accumulate( DWS2pb, DWS2pe, WorkerSubClass2<T>(dZero), myTypeName + " accumulate WorkerSubClass2 pointer");
    test_accumulate( DWS3pb, DWS3pe, WorkerSubClass3<T>(dZero), myTypeName + " accumulate WorkerSubClass3 pointer");

    std::string temp1( myTypeName + " Inheritance Accumulate");
    summarize( temp1.c_str(), SIZE, iterations, kShowGMeans, kShowPenalty );


    // the sorting tests are much slower than the accumulation tests - O(N^2)
    iterations = iterations / 1600;
    
    // fill one set of random numbers
    fill_random( dMpb, dMpe );
    // copy to the other sets, so we have the same numbers
    ::copy( dMpb, dMpe, DMpb );
    ::copy( dMpb, dMpe, DSMpb );
    ::copy( dMpb, dMpe, DS2Mpb );
    ::copy( dMpb, dMpe, DDMpb );
    ::copy( dMpb, dMpe, DVMpb );
    ::copy( dMpb, dMpe, DVSMpb );
    ::copy( dMpb, dMpe, DWSMpb );
    ::copy( dMpb, dMpe, DWDMpb );
    ::copy( dMpb, dMpe, DWSM2pb );
    ::copy( dMpb, dMpe, DWSM3pb );

    test_insertion_sort( dMpb, dMpe, dpb, dpe, dZero, myTypeName + " insertion_sort pointer verify1");
    test_insertion_sort( DMpb, DMpe, Dpb, Dpe, DZero, myTypeName + " insertion_sort TypeClass pointer verify1");
    test_insertion_sort( DSMpb, DSMpe, DSpb, DSpe, TypeSubClass<T>(dZero), myTypeName + " insertion_sort TypeSubClass pointer");
    test_insertion_sort( DS2Mpb, DS2Mpe, DS2pb, DS2pe, TypeSub2Class<T>(dZero), myTypeName + " insertion_sort TypeSub2Class pointer");
    test_insertion_sort( DDMpb, DDMpe, DDpb, DDpe, DeepSubClass<T>(dZero), myTypeName + " insertion_sort DeepSubClass pointer");
    test_insertion_sort( DVMpb, DVMpe, DVpb, DVpe, TypeVirtualClass<T>(dZero), myTypeName + " insertion_sort TypeVirtualClass pointer");
    test_insertion_sort( DVSMpb, DVSMpe, DVSpb, DVSpe, TypeVirtualSubClass<T>(dZero), myTypeName + " insertion_sort TypeVirtualSubClass pointer");
    test_insertion_sort( DWSMpb, DWSMpe, DWSpb, DWSpe, WorkerSubClass<T>(dZero), myTypeName + " insertion_sort WorkerSubClass pointer");
    test_insertion_sort( DWDMpb, DWDMpe, DWDpb, DWDpe, WorkerDeepSubClass<T>(dZero), myTypeName + " insertion_sort WorkerDeepSubClass pointer");
    test_insertion_sort( DWSM2pb, DWSM2pe, DWS2pb, DWS2pe, WorkerSubClass2<T>(dZero), myTypeName + " insertion_sort WorkerSubClass2 pointer");
    test_insertion_sort( DWSM3pb, DWSM3pe, DWS3pb, DWS3pe, WorkerSubClass3<T>(dZero), myTypeName + " insertion_sort WorkerSubClass3 pointer");

    std::string temp2( myTypeName + " Inheritance Insertion Sort");
    summarize( temp2.c_str(), SIZE, iterations, kShowGMeans, kShowPenalty );


    // these are slightly faster - O(NLog2(N))
    iterations = iterations * 16;

    test_quicksort( dMpb, dMpe, dpb, dpe, dZero, myTypeName + " quicksort pointer verify1");
    test_quicksort( DMpb, DMpe, Dpb, Dpe, DZero, myTypeName + " quicksort TypeClass pointer verify1");
    test_quicksort( DSMpb, DSMpe, DSpb, DSpe, TypeSubClass<T>(dZero), myTypeName + " quicksort TypeSubClass pointer");
    test_quicksort( DS2Mpb, DS2Mpe, DS2pb, DS2pe, TypeSub2Class<T>(dZero), myTypeName + " quicksort TypeSub2Class pointer");
    test_quicksort( DDMpb, DDMpe, DDpb, DDpe, DeepSubClass<T>(dZero), myTypeName + " quicksort DeepSubClass pointer");
    test_quicksort( DVMpb, DVMpe, DVpb, DVpe, TypeVirtualClass<T>(dZero), myTypeName + " quicksort TypeVirtualClass pointer");
    test_quicksort( DVSMpb, DVSMpe, DVSpb, DVSpe, TypeVirtualSubClass<T>(dZero), myTypeName + " quicksort TypeVirtualSubClass pointer");
    test_quicksort( DWSMpb, DWSMpe, DWSpb, DWSpe, WorkerSubClass<T>(dZero), myTypeName + " quicksort WorkerSubClass pointer");
    test_quicksort( DWDMpb, DWDMpe, DWDpb, DWDpe, WorkerDeepSubClass<T>(dZero), myTypeName + " quicksort WorkerDeepSubClass pointer");
    test_quicksort( DWSM2pb, DWSM2pe, DWS2pb, DWS2pe, WorkerSubClass2<T>(dZero), myTypeName + " quicksort WorkerSubClass2 pointer");
    test_quicksort( DWSM3pb, DWSM3pe, DWS3pb, DWS3pe, WorkerSubClass3<T>(dZero), myTypeName + " quicksort WorkerSubClass3 pointer");

    std::string temp3( myTypeName + " Inheritance Quicksort");
    summarize( temp3.c_str(), SIZE, iterations, kShowGMeans, kShowPenalty );


    test_heap_sort( dMpb, dMpe, dpb, dpe, dZero, myTypeName + " heap_sort pointer verify1");
    test_heap_sort( DMpb, DMpe, Dpb, Dpe, DZero, myTypeName + " heap_sort TypeClass pointer verify1");
    test_heap_sort( DSMpb, DSMpe, DSpb, DSpe, TypeSubClass<T>(dZero), myTypeName + " heap_sort TypeSubClass pointer");
    test_heap_sort( DS2Mpb, DS2Mpe, DS2pb, DS2pe, TypeSub2Class<T>(dZero), myTypeName + " heap_sort TypeSub2Class pointer");
    test_heap_sort( DDMpb, DDMpe, DDpb, DDpe, DeepSubClass<T>(dZero), myTypeName + " heap_sort DeepSubClass pointer");
    test_heap_sort( DVMpb, DVMpe, DVpb, DVpe, TypeVirtualClass<T>(dZero), myTypeName + " heap_sort TypeVirtualClass pointer");
    test_heap_sort( DVSMpb, DVSMpe, DVSpb, DVSpe, TypeVirtualSubClass<T>(dZero), myTypeName + " heap_sort TypeVirtualSubClass pointer");
    test_heap_sort( DWSMpb, DWSMpe, DWSpb, DWSpe, WorkerSubClass<T>(dZero), myTypeName + " heap_sort WorkerSubClass pointer");
    test_heap_sort( DWDMpb, DWDMpe, DWDpb, DWDpe, WorkerDeepSubClass<T>(dZero), myTypeName + " heap_sort WorkerDeepSubClass pointer");
    test_heap_sort( DWSM2pb, DWSM2pe, DWS2pb, DWS2pe, WorkerSubClass2<T>(dZero), myTypeName + " heap_sort WorkerSubClass2 pointer");
    test_heap_sort( DWSM3pb, DWSM3pe, DWS3pb, DWS3pe, WorkerSubClass3<T>(dZero), myTypeName + " heap_sort WorkerSubClass3 pointer");
    
    std::string temp4( myTypeName + " Inheritance Heap Sort");
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


    iterations *= 4;
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
