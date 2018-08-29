/*
    Copyright 2007-2009 Adobe Systems Incorporated
    Copyright 2018 Chris Coxs
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
#include "benchmark_results.h"
#include "benchmark_timer.h"
#include "benchmark_algorithms.h"

/******************************************************************************/

// a single double value wrapped in a class

struct DoubleClass {
    double value;
    DoubleClass() {}
    DoubleClass(const double& x) : value(x) {}
    operator double() { return value; }
};

inline DoubleClass operator+(const DoubleClass& x, const DoubleClass& y) {
    return DoubleClass(x.value + y.value);
}

/******************************************************************************/

// a subclass of DoubleClass, with no changes that interfere with the parent class
struct DoubleSubClass : public DoubleClass {
    DoubleSubClass() {}
    DoubleSubClass(const double& x) : DoubleClass(x) {}

    // this function is unused in this test - this is just to add something beyond the parent class
    inline DoubleSubClass do_nothing (const DoubleSubClass& x, const DoubleSubClass& y) {
        return DoubleSubClass(x.value * y.value);
    }
};

inline DoubleSubClass operator+(const DoubleSubClass& x, const DoubleSubClass& y) {
    return DoubleSubClass(x.value + y.value);
}

/******************************************************************************/

// a subclass of DoubleSubClass, with no changes that interfere with the parent class
struct DoubleSub2Class : public DoubleSubClass {
    DoubleSub2Class() {}
    DoubleSub2Class(const double& x) : DoubleSubClass(x) {}

    // this function is unused in this test - this is just to add something beyond the parent class
    inline DoubleSub2Class do_nothing2 (const DoubleSub2Class& x, const DoubleSub2Class& y) {
        return DoubleSub2Class(x.value * y.value);
    }
};

inline DoubleSub2Class operator+(const DoubleSub2Class& x, const DoubleSub2Class& y) {
    return DoubleSub2Class(x.value + y.value);
}

/******************************************************************************/

// a "do nothing" parent class
struct EmptyClass {
    EmptyClass() {}
    EmptyClass(const double& x) {}
};

// a subclass that does nothing
struct EmptyClass1 : public EmptyClass {
    EmptyClass1() {}
    EmptyClass1(const double& x) : EmptyClass(x) {}
};

// a subclass that does nothing
struct EmptyClass2 : public EmptyClass1 {
    EmptyClass2() {}
    EmptyClass2(const double& x) : EmptyClass1(x) {}
};

// a subclass that does nothing
struct EmptyClass3 : public EmptyClass2 {
    EmptyClass3() {}
    EmptyClass3(const double& x) : EmptyClass2(x) {}
};

// a subclass that does nothing
struct EmptyClass4 : public EmptyClass3 {
    EmptyClass4() {}
    EmptyClass4(const double& x) : EmptyClass3(x) {}
};

// a child class that holds data and does the work
// exact type usage is known at compile time
// accessor is monomorphic
struct DeepSubClass : public EmptyClass4 {
    double value;
    DeepSubClass() {}
    DeepSubClass(const double& x) : EmptyClass4(x), value(x) {}
    operator double() { return value; }
};

inline DeepSubClass operator+(const DeepSubClass& x, const DeepSubClass& y) {
    return DeepSubClass(x.value + y.value);
}

/******************************************************************************/

// a single double value wrapped in a class, with a virtual accessor
// there is no subclass, so the exact type is known at compile time
// there is no override, so the accessor is monomorphic, no VTable access needed
struct DoubleVirtualClass {
    double value;

    DoubleVirtualClass() {}
    DoubleVirtualClass(const double& x) : value(x) {}

    virtual operator double() { return value; }
};

inline DoubleVirtualClass operator+(const DoubleVirtualClass& x, const DoubleVirtualClass& y) {
    return DoubleVirtualClass(x.value + y.value);
}

/******************************************************************************/

// a single double value wrapped in a class, with a virtual accessor
struct DoubleVirtualParentClass {
    double value;

    DoubleVirtualParentClass() {}
    DoubleVirtualParentClass(const double& x) : value(x) {}

    // this accessor will be unused
    virtual operator double() { return -1.0; }
};

// a subclass of DoubleVirtualParentClass
// exact type usage is known at compile time
// accessor is polymorphic, but can be resolved exactly, no VTable access needed
struct DoubleVirtualSubClass : public DoubleVirtualParentClass {
    DoubleVirtualSubClass() {}
    DoubleVirtualSubClass(const double& x) : DoubleVirtualParentClass(x) {}

    // only this accessor can be used
    virtual operator double() { return value; }

    // this function is unused in this test - this is just to add something beyond the parent class
    virtual DoubleVirtualSubClass do_mult (const DoubleVirtualSubClass& x, const DoubleVirtualSubClass& y) {
        return DoubleVirtualSubClass(x.value * y.value);
    }
};

inline DoubleVirtualSubClass operator+(const DoubleVirtualSubClass& x, const DoubleVirtualSubClass& y) {
    return DoubleVirtualSubClass(x.value + y.value);
}

/******************************************************************************/

// a "do nothing" parent class with a virtual accessor
struct InterfaceClass {
    InterfaceClass() {}
    InterfaceClass(const double& x) {}
    virtual operator double() { return 0.0; }
};

inline InterfaceClass operator+(const InterfaceClass& x, const InterfaceClass& y) {
    return InterfaceClass(2.0);
}

// a child class that holds data and does the work
// exact type usage is known at compile time
// accessor is polymorphic, but can be resolved exactly, no VTable access needed
struct WorkerSubClass : public InterfaceClass {
    double value;
    WorkerSubClass() {}
    WorkerSubClass(const double& x) : InterfaceClass(x), value(x) {}
    virtual operator double() { return value; }
};

inline WorkerSubClass operator+(const WorkerSubClass& x, const WorkerSubClass& y) {
    return WorkerSubClass(x.value + y.value);
}

/******************************************************************************/

// a "do nothing" parent class with a virtual accessor
struct InterfaceClassB {
    InterfaceClassB() {}
    InterfaceClassB(const double& x) {}
    virtual operator double() { return 0.0; }
};

inline InterfaceClassB operator+(const InterfaceClassB& x, const InterfaceClassB& y) {
    return InterfaceClassB(2.0);
}

// a subclass that does nothing
struct InterfaceClass1 : public InterfaceClassB {
    InterfaceClass1() {}
    InterfaceClass1(const double& x) :InterfaceClassB(x) {}
};

// a subclass that does nothing
struct InterfaceClass2 : public InterfaceClass1 {
    InterfaceClass2() {}
    InterfaceClass2(const double& x) : InterfaceClass1(x) {}
    virtual operator double() { return 99.0; }
};

// a subclass that does nothing
struct InterfaceClass3 : public InterfaceClass2 {
    InterfaceClass3() {}
    InterfaceClass3(const double& x) : InterfaceClass2(x) {}
};

// a subclass that does nothing
struct InterfaceClass4 : public InterfaceClass3 {
    InterfaceClass4() {}
    InterfaceClass4(const double& x) : InterfaceClass3(x) {}
    virtual operator double() { return 97.0; }
};

// a child class that holds data and does the work
// exact type usage is known at compile time
// accessor is polymorphic, but can be resolved exactly, no VTable access needed
struct WorkerDeepSubClass : public InterfaceClass4 {
    double value;
    WorkerDeepSubClass() {}
    WorkerDeepSubClass(const double& x) : InterfaceClass4(x), value(x) {}
    virtual operator double() { return value; }
};

inline WorkerDeepSubClass operator+(const WorkerDeepSubClass& x, const WorkerDeepSubClass& y) {
    return WorkerDeepSubClass(x.value + y.value);
}

/******************************************************************************/

// a "do nothing" abstract parent class with a virtual accessor
struct InterfaceClassA {
    InterfaceClassA() {}
    InterfaceClassA(const double& x) {}
    virtual operator double() = 0;  // explicitly state that this cannot be called
};

// a child class that holds data and does the work
// exact type usage is known at compile time
// accessor is polymorphic, but can be resolved exactly, no VTable access needed
struct WorkerSubClass2 : public InterfaceClassA {
    double value;
    WorkerSubClass2() {}
    WorkerSubClass2(const double& x) : InterfaceClassA(x), value(x) {}
    virtual operator double() { return value; }
};

inline WorkerSubClass2 operator+(const WorkerSubClass2& x, const WorkerSubClass2& y) {
    return WorkerSubClass2(x.value + y.value);
}

/******************************************************************************/

// a "do nothing" abstract parent class with a virtual accessor
struct InterfaceClassC {
    InterfaceClassC() {}
    InterfaceClassC(const double& x) {}
    virtual operator double() { return 0.0; }
};

// a child class that holds data and does the work
// exact type usage is known at compile time
// accessor is polymorphic (with siblings), but can be resolved exactly, no VTable access needed
struct WorkerSubClass3 : public InterfaceClassC {
    double value;
    WorkerSubClass3() {}
    WorkerSubClass3(const double& x) : InterfaceClassC(x), value(x) {}
    virtual int do_nothing3() { return 3; }
    virtual operator double() { return value; }
};

inline WorkerSubClass3 operator+(const WorkerSubClass3& x, const WorkerSubClass3& y) {
    return WorkerSubClass3(x.value + y.value);
}

// a child class that holds data and does the work
// exact type usage is known at compile time
// accessor is polymorphic (with siblings), but can be resolved exactly, no VTable access needed
struct WorkerSubClass4 : public InterfaceClassC {
    double value;
    WorkerSubClass4() {}
    WorkerSubClass4(const double& x) : InterfaceClassC(x), value(x) {}
    virtual double do_nothing4() { return 4.0; }
    virtual operator double() { return value; }
};

inline WorkerSubClass4 operator+(const WorkerSubClass4& x, const WorkerSubClass4& y) {
    return WorkerSubClass4(x.value + y.value);
}

// a child class that holds data and does the work
// exact type usage is known at compile time
// accessor is polymorphic (with siblings), but can be resolved exactly, no VTable access needed
struct WorkerSubClass5 : public InterfaceClassC {
    double value;
    WorkerSubClass5() {}
    WorkerSubClass5(const double& x) : InterfaceClassC(x), value(x) {}
    virtual void do_nothing5() {}
    virtual operator double() { return value; }
};

inline WorkerSubClass5 operator+(const WorkerSubClass5& x, const WorkerSubClass5& y) {
    return WorkerSubClass5(x.value + y.value);
}

/******************************************************************************/
/******************************************************************************/

// this constant may need to be adjusted to give reasonable minimum times
// For best results, times should be about 1.0 seconds for the minimum test run
int iterations = 400000;

// 4000 items, or about 32k of data
// this is intended to remain within the L2 cache of most common CPUs
const int SIZE = 4000;

// initial value for filling our arrays, may be changed from the command line
double init_value = 3.0;

/******************************************************************************/
/******************************************************************************/

inline void check_sum(double result, const char *label) {
  if (result != SIZE * init_value)
    printf("test %s failed\n", label);
}

/******************************************************************************/

template <typename Iterator>
void verify_sorted(Iterator first, Iterator last, const char *label) {
    if (!is_sorted(first,last))
        printf("sort test %s failed\n", label);
}

/******************************************************************************/

template <typename Iterator, typename T>
void test_accumulate(Iterator first, Iterator last, T zero, const char *label) {
  
  start_timer();
  
  for(int i = 0; i < iterations; ++i)
    check_sum( double( accumulate(first, last, zero) ), label );
    
  record_result( timer(), label );
}

/******************************************************************************/

template <typename Iterator, typename T>
void test_insertion_sort(Iterator firstSource, Iterator lastSource, Iterator firstDest,
                        Iterator lastDest, T zero, const char *label) {

    start_timer();

    for(int i = 0; i < iterations; ++i) {
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

    start_timer();

    for(int i = 0; i < iterations; ++i) {
        ::copy(firstSource, lastSource, firstDest);
        quicksort< Iterator, T>( firstDest, lastDest );
        verify_sorted( firstDest, lastDest, label );
    }
    
    record_result( timer(), label );
}

/******************************************************************************/

template <typename Iterator, typename T>
void test_heap_sort(Iterator firstSource, Iterator lastSource, Iterator firstDest,
                    Iterator lastDest, T zero, const char *label) {

    start_timer();

    for(int i = 0; i < iterations; ++i) {
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
DoubleClass Data[SIZE];
DoubleSubClass DSData[SIZE];
DoubleSub2Class DS2Data[SIZE];
DeepSubClass DeepData[SIZE];
DoubleVirtualClass DVData[SIZE];
DoubleVirtualSubClass DVSData[SIZE];
WorkerSubClass WSData[SIZE];
WorkerDeepSubClass WDData[SIZE];
WorkerSubClass2 WSData2[SIZE];
WorkerSubClass3 WSData3[SIZE];

double dataMaster[SIZE];
DoubleClass DataMaster[SIZE];
DoubleSubClass DSDataMaster[SIZE];
DoubleSub2Class DS2DataMaster[SIZE];
DeepSubClass DeepDataMaster[SIZE];
DoubleVirtualClass DVDataMaster[SIZE];
DoubleVirtualSubClass DVSDataMaster[SIZE];
WorkerSubClass WSDataMaster[SIZE];
WorkerDeepSubClass WDDataMaster[SIZE];
WorkerSubClass2 WSDataMaster2[SIZE];
WorkerSubClass3 WSDataMaster3[SIZE];

/******************************************************************************/

// declaration of our iterator types and begin/end pairs
typedef double* dp;
dp dpb = data;
dp dpe = data + SIZE;
dp dMpb = dataMaster;
dp dMpe = dataMaster + SIZE;

typedef DoubleClass* Dp;
Dp Dpb = Data;
Dp Dpe = Data + SIZE;
Dp DMpb = DataMaster;
Dp DMpe = DataMaster + SIZE;

typedef DoubleSubClass* DSp;
DSp DSpb = DSData;
DSp DSpe = DSData + SIZE;
DSp DSMpb = DSDataMaster;
DSp DSMpe = DSDataMaster + SIZE;

typedef DoubleSub2Class* DS2p;
DS2p DS2pb = DS2Data;
DS2p DS2pe = DS2Data + SIZE;
DS2p DS2Mpb = DS2DataMaster;
DS2p DS2Mpe = DS2DataMaster + SIZE;

typedef DeepSubClass* DDp;
DDp DDpb = DeepData;
DDp DDpe = DeepData + SIZE;
DDp DDMpb = DeepDataMaster;
DDp DDMpe = DeepDataMaster + SIZE;

typedef DoubleVirtualClass* DVp;
DVp DVpb = DVData;
DVp DVpe = DVData + SIZE;
DVp DVMpb = DVDataMaster;
DVp DVMpe = DVDataMaster + SIZE;

typedef DoubleVirtualSubClass* DVSp;
DVSp DVSpb = DVSData;
DVSp DVSpe = DVSData + SIZE;
DVSp DVSMpb = DVSDataMaster;
DVSp DVSMpe = DVSDataMaster + SIZE;

typedef WorkerSubClass* DWSp;
DWSp DWSpb = WSData;
DWSp DWSpe = WSData + SIZE;
DWSp DWSMpb = WSDataMaster;
DWSp DWSMpe = WSDataMaster + SIZE;

typedef WorkerDeepSubClass* DWDp;
DWDp DWDpb = WDData;
DWDp DWDpe = WDData + SIZE;
DWDp DWDMpb = WDDataMaster;
DWDp DWDMpe = WDDataMaster + SIZE;

typedef WorkerSubClass2* DWS2p;
DWS2p DWS2pb = WSData2;
DWS2p DWS2pe = WSData2 + SIZE;
DWS2p DWSM2pb = WSDataMaster2;
DWS2p DWSM2pe = WSDataMaster2 + SIZE;

typedef WorkerSubClass3* DWS3p;
DWS3p DWS3pb = WSData3;
DWS3p DWS3pe = WSData3 + SIZE;
DWS3p DWSM3pb = WSDataMaster3;
DWS3p DWSM3pe = WSDataMaster3 + SIZE;

/******************************************************************************/
/******************************************************************************/

int main(int argc, char** argv) {

    double dZero = 0.0;
    DoubleClass DZero = 0.0;

    // output command for documentation:
    int i;
    for (i = 0; i < argc; ++i)
        printf("%s ", argv[i] );
    printf("\n");

    if (argc > 1) iterations = atoi(argv[1]);
    if (argc > 2) init_value = (double) atof(argv[2]);

    // seed the random number generator so we get repeatable results
    srand( (int)init_value + 123 );

 
    fill( dpb, dpe, double(init_value) );
    fill( Dpb, Dpe, DoubleClass(init_value) );
    fill( DSpb, DSpe, DoubleSubClass(init_value) );
    fill( DS2pb, DS2pe, DoubleSub2Class(init_value) );
    fill( DVpb, DVpe, DoubleVirtualClass(init_value) );
    fill( DVSpb, DVSpe, DoubleVirtualSubClass(init_value) );
    fill( DWSpb, DWSpe, WorkerSubClass(init_value) );
    fill( DWDpb, DWDpe, WorkerDeepSubClass(init_value) );
    fill( DDpb, DDpe, DeepSubClass(init_value) );
    fill( DWS2pb, DWS2pe, WorkerSubClass2(init_value) );
    fill( DWS3pb, DWS3pe, WorkerSubClass3(init_value) );

    test_accumulate( dpb, dpe, dZero, "accumulate double pointer verify1");
    test_accumulate( Dpb, Dpe, DoubleClass(dZero), "accumulate DoubleClass pointer verify1");
    test_accumulate( DSpb, DSpe, DoubleSubClass(dZero), "accumulate DoubleSubClass pointer");
    test_accumulate( DS2pb, DS2pe, DoubleSub2Class(dZero), "accumulate DoubleSub2Class pointer");
    test_accumulate( DDpb, DDpe, DeepSubClass(dZero), "accumulate DeepSubClass pointer");
    test_accumulate( DVpb, DVpe, DoubleVirtualClass(dZero), "accumulate DoubleVirtualClass pointer");
    test_accumulate( DVSpb, DVSpe, DoubleVirtualSubClass(dZero), "accumulate DoubleVirtualSubClass pointer");
    test_accumulate( DWSpb, DWSpe, WorkerSubClass(dZero), "accumulate WorkerSubClass pointer");
    test_accumulate( DWDpb, DWDpe, WorkerDeepSubClass(dZero), "accumulate WorkerDeepSubClass pointer");
    test_accumulate( DWS2pb, DWS2pe, WorkerSubClass2(dZero), "accumulate WorkerSubClass2 pointer");
    test_accumulate( DWS3pb, DWS3pe, WorkerSubClass3(dZero), "accumulate WorkerSubClass3 pointer");

    summarize("Inheritance Accumulate", SIZE, iterations, kShowGMeans, kShowPenalty );


    // the sorting tests are much slower than the accumulation tests - O(N^2)
    iterations = iterations / 1600;
    
    // fill one set of random numbers
    fill_random<double *, double>( dMpb, dMpe );
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

    test_insertion_sort( dMpb, dMpe, dpb, dpe, dZero, "insertion_sort double pointer verify1");
    test_insertion_sort( DMpb, DMpe, Dpb, Dpe, DZero, "insertion_sort DoubleClass pointer verify1");
    test_insertion_sort( DSMpb, DSMpe, DSpb, DSpe, DoubleSubClass(dZero), "insertion_sort DoubleSubClass pointer");
    test_insertion_sort( DS2Mpb, DS2Mpe, DS2pb, DS2pe, DoubleSub2Class(dZero), "insertion_sort DoubleSub2Class pointer");
    test_insertion_sort( DDMpb, DDMpe, DDpb, DDpe, DeepSubClass(dZero), "insertion_sort DeepSubClass pointer");
    test_insertion_sort( DVMpb, DVMpe, DVpb, DVpe, DoubleVirtualClass(dZero), "insertion_sort DoubleVirtualClass pointer");
    test_insertion_sort( DVSMpb, DVSMpe, DVSpb, DVSpe, DoubleVirtualSubClass(dZero), "insertion_sort DoubleVirtualSubClass pointer");
    test_insertion_sort( DWSMpb, DWSMpe, DWSpb, DWSpe, WorkerSubClass(dZero), "insertion_sort WorkerSubClass pointer");
    test_insertion_sort( DWDMpb, DWDMpe, DWDpb, DWDpe, WorkerDeepSubClass(dZero), "insertion_sort WorkerDeepSubClass pointer");
    test_insertion_sort( DWSM2pb, DWSM2pe, DWS2pb, DWS2pe, WorkerSubClass2(dZero), "insertion_sort WorkerSubClass2 pointer");
    test_insertion_sort( DWSM3pb, DWSM3pe, DWS3pb, DWS3pe, WorkerSubClass3(dZero), "insertion_sort WorkerSubClass3 pointer");

    summarize("Inheritance Insertion Sort", SIZE, iterations, kShowGMeans, kShowPenalty );


    // these are slightly faster - O(NLog2(N))
    iterations = iterations * 16;

    test_quicksort( dMpb, dMpe, dpb, dpe, dZero, "quicksort double pointer verify1");
    test_quicksort( DMpb, DMpe, Dpb, Dpe, DZero, "quicksort DoubleClass pointer verify1");
    test_quicksort( DSMpb, DSMpe, DSpb, DSpe, DoubleSubClass(dZero), "quicksort DoubleSubClass pointer");
    test_quicksort( DS2Mpb, DS2Mpe, DS2pb, DS2pe, DoubleSub2Class(dZero), "quicksort DoubleSub2Class pointer");
    test_quicksort( DDMpb, DDMpe, DDpb, DDpe, DeepSubClass(dZero), "quicksort DeepSubClass pointer");
    test_quicksort( DVMpb, DVMpe, DVpb, DVpe, DoubleVirtualClass(dZero), "quicksort DoubleVirtualClass pointer");
    test_quicksort( DVSMpb, DVSMpe, DVSpb, DVSpe, DoubleVirtualSubClass(dZero), "quicksort DoubleVirtualSubClass pointer");
    test_quicksort( DWSMpb, DWSMpe, DWSpb, DWSpe, WorkerSubClass(dZero), "quicksort WorkerSubClass pointer");
    test_quicksort( DWDMpb, DWDMpe, DWDpb, DWDpe, WorkerDeepSubClass(dZero), "quicksort WorkerDeepSubClass pointer");
    test_quicksort( DWSM2pb, DWSM2pe, DWS2pb, DWS2pe, WorkerSubClass2(dZero), "quicksort WorkerSubClass2 pointer");
    test_quicksort( DWSM3pb, DWSM3pe, DWS3pb, DWS3pe, WorkerSubClass3(dZero), "quicksort WorkerSubClass3 pointer");

    summarize("Inheritance Quicksort", SIZE, iterations, kShowGMeans, kShowPenalty );


    test_heap_sort( dMpb, dMpe, dpb, dpe, dZero, "heap_sort double pointer verify1");
    test_heap_sort( DMpb, DMpe, Dpb, Dpe, DZero, "heap_sort DoubleClass pointer verify1");
    test_heap_sort( DSMpb, DSMpe, DSpb, DSpe, DoubleSubClass(dZero), "heap_sort DoubleSubClass pointer");
    test_heap_sort( DS2Mpb, DS2Mpe, DS2pb, DS2pe, DoubleSub2Class(dZero), "heap_sort DoubleSub2Class pointer");
    test_heap_sort( DDMpb, DDMpe, DDpb, DDpe, DeepSubClass(dZero), "heap_sort DeepSubClass pointer");
    test_heap_sort( DVMpb, DVMpe, DVpb, DVpe, DoubleVirtualClass(dZero), "heap_sort DoubleVirtualClass pointer");
    test_heap_sort( DVSMpb, DVSMpe, DVSpb, DVSpe, DoubleVirtualSubClass(dZero), "heap_sort DoubleVirtualSubClass pointer");
    test_heap_sort( DWSMpb, DWSMpe, DWSpb, DWSpe, WorkerSubClass(dZero), "heap_sort WorkerSubClass pointer");
    test_heap_sort( DWDMpb, DWDMpe, DWDpb, DWDpe, WorkerDeepSubClass(dZero), "heap_sort WorkerDeepSubClass pointer");
    test_heap_sort( DWSM2pb, DWSM2pe, DWS2pb, DWS2pe, WorkerSubClass2(dZero), "heap_sort WorkerSubClass2 pointer");
    test_heap_sort( DWSM3pb, DWSM3pe, DWS3pb, DWS3pe, WorkerSubClass3(dZero), "heap_sort WorkerSubClass3 pointer");
    
    summarize("Inheritance Heap Sort", SIZE, iterations, kShowGMeans, kShowPenalty );
    
    
    return 0;
}

// the end
/******************************************************************************/
/******************************************************************************/
