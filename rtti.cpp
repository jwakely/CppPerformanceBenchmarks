/*
    Copyright 2007-2008 Adobe Systems Incorporated
    Copyright 2018 Chris Cox
    Distributed under the MIT License (see accompanying file LICENSE_1_0_0.txt
    or a copy at http://stlab.adobe.com/licenses.html )


Goal: Examine the performance of run time type information (RTTI)
        and compare to hand coded implementations.

Assumptions:

    1) typeid name comparisons will perform close to using a type name in a struct.
    
    2) typeid type_info comparisons will perform close to using a type value in a struct.
        Won't be quite as efficient because we can't use a switch statement.

*/

#include <stddef.h>
#include <stdio.h>
#include <time.h>
#include <math.h>
#include <stdlib.h>
#include <typeinfo>
#if WIN32
#include <string.h>
#else
#include <strings.h>
#endif
#include "benchmark_timer.h"
#include "benchmark_results.h"

/******************************************************************************/


// this constant may need to be adjusted to give reasonable minimum times
// For best results, times should be about 1.0 seconds for the minimum test run
// on 3Ghz desktop CPUs, 25k iterations is about 1.8 seconds
int iterations = 25000;


#define SIZE     4000


// initial value for filling our arrays, may be changed from the command line
double init_value = 3.0;

// initial value for simple random number generator
unsigned int seed_value = 42;

/******************************************************************************/
/******************************************************************************/

// leaf type name is always first member in struct
// so  ((typeValue *) &struct) will give the leaf type
// and the leaf type must know it's parent type

typedef enum {
    typeValueParent,
    typeValueA,
    typeValueB,
    typeValueC
} typeValue;

typedef struct {
    typeValue type_value;
    double value;
} simpleValueParent;

typedef struct {
    typeValue type_value;
    simpleValueParent parent;
    int another_value;
} simpleValueA;

typedef struct {
    typeValue type_value;
    float another_value;
    simpleValueParent parent;
} simpleValueB;

typedef struct {
    typeValue type_value;
    simpleValueParent parent;
    float another_value1;
    float another_value2;
} simpleValueC;


void initSimpleValueParent(simpleValueParent &sp) {
    sp.value = init_value;
    sp.type_value = typeValueParent;
}

simpleValueA *makeSimpleValueA() {
    simpleValueA *result = new simpleValueA;
    initSimpleValueParent( result->parent );
    result->another_value = 2;
    result->type_value = typeValueA;
    return result;
}

simpleValueB *makeSimpleValueB() {
    simpleValueB *result = new simpleValueB;
    initSimpleValueParent( result->parent );
    result->another_value = 2.0;
    result->type_value = typeValueB;
    return result;
}

simpleValueC *makeSimpleValueC() {
    simpleValueC *result = new simpleValueC;
    initSimpleValueParent( result->parent );
    result->another_value1 = 3.0;
    result->another_value2 = 4.0;
    result->type_value = typeValueC;
    return result;
}


/******************************************************************************/
/******************************************************************************/

// leaf type name is always first member in struct
// so  ((const char *) &struct) will give the leaf type
// and the leaf type must know it's parent type

typedef struct {
    const char *type_name;
    double value;
} simpleStringParent;

typedef struct {
    const char *type_name;
    simpleStringParent parent;
    int another_value;
} simpleStringA;

typedef struct {
    const char *type_name;
    float another_value;
    simpleStringParent parent;
} simpleStringB;

typedef struct {
    const char *type_name;
    simpleStringParent parent;
    float another_value1;
    float another_value2;
} simpleStringC;


void initSimpleStringParent(simpleStringParent &sp) {
    sp.value = init_value;
    sp.type_name = "simpleStringParent";
}

simpleStringA *makeSimpleStringA() {
    simpleStringA *result = new simpleStringA;
    initSimpleStringParent( result->parent );
    result->another_value = 2;
    result->type_name = "simpleStringA";
    return result;
}

simpleStringB *makeSimpleStringB() {
    simpleStringB *result = new simpleStringB;
    initSimpleStringParent( result->parent );
    result->another_value = 2.0;
    result->type_name = "simpleStringB";
    return result;
}

simpleStringC *makeSimpleStringC() {
    simpleStringC *result = new simpleStringC;
    initSimpleStringParent( result->parent );
    result->another_value1 = 3.0;
    result->another_value2 = 4.0;
    result->type_name = "simpleStringC";
    return result;
}

/******************************************************************************/


class parentClass {
    public:
        parentClass() : value(init_value) {}
        
        // virtual functions so the classes have typeID, and can't be optimized away completely
        virtual int unused() { return 99; }
    public:
        double value;
};

class subclassA : public parentClass {
    public:
        subclassA() : another_value(2) {}
        
        virtual int unused() { return 0; }
    public:
        int another_value;
};

class subclassB : public parentClass {
    public:
        subclassB() : another_value(2.0) {}
        
        virtual int unused() { return 2; }
    public:
        float another_value;
};

class subclassC : public parentClass {
    public:
        subclassC() : another_value1(3.0), another_value2(4.0) {}
        
        virtual int unused() { return 4;}
    public:
        int another_value1;
        int another_value2;
};


// Optionally test the impact of the number of releated classes on RTTI performance.
// So far, the impact is almost zero on the compilers tested.
#ifndef TEST_MORE_CLASSES
#define TEST_MORE_CLASSES    0
#endif

#if TEST_MORE_CLASSES

#define quickClass(X)    \
class subclass##X : public parentClass {    \
    public:    \
        subclass##X () : another_value(2) {}    \
        virtual int unused() { return 0; }    \
        int another_value;    \
};

#define quickClassTenPack(X)    \
    quickClass(Z1##X##0)    \
    quickClass(Z1##X##1)    \
    quickClass(Z1##X##2)    \
    quickClass(Z1##X##3)    \
    quickClass(Z1##X##4)    \
    quickClass(Z1##X##5)    \
    quickClass(Z1##X##6)    \
    quickClass(Z1##X##7)    \
    quickClass(Z1##X##8)    \
    quickClass(Z1##X##9)

#define quickClassHundredPack(K)    \
    quickClassTenPack(K##0)    \
    quickClassTenPack(K##1)    \
    quickClassTenPack(K##2)    \
    quickClassTenPack(K##3)    \
    quickClassTenPack(K##4)    \
    quickClassTenPack(K##5)    \
    quickClassTenPack(K##6)    \
    quickClassTenPack(K##7)    \
    quickClassTenPack(K##8)    \
    quickClassTenPack(K##9)

#define quickClassThousandPack(M)    \
    quickClassHundredPack(M##0)    \
    quickClassHundredPack(M##1)    \
    quickClassHundredPack(M##2)    \
    quickClassHundredPack(M##3)    \
    quickClassHundredPack(M##4)    \
    quickClassHundredPack(M##5)    \
    quickClassHundredPack(M##6)    \
    quickClassHundredPack(M##7)    \
    quickClassHundredPack(M##8)    \
    quickClassHundredPack(M##9)

// create two thousand sub classes
quickClassThousandPack(0)
quickClassThousandPack(1)

#endif    // TEST_MORE_CLASSES

/******************************************************************************/
/******************************************************************************/

simpleStringParent *simpleStringData[SIZE];
simpleValueParent *simpleValueData[SIZE];
parentClass *classData[SIZE];

// how many of each type was created
int gCountA = 0;
int gCountB = 0;
int gCountC = 0;

/******************************************************************************/

inline unsigned int bad_hash(unsigned int x) {
    return ((4237 * x) + 12345);
}

typedef int reducer_func( unsigned int input );

int reduce_to_3( unsigned int input ) {
    return input % 3;
}

/******************************************************************************/

void createSimpleValueData(reducer_func *process) {
    int i;
    unsigned int selector = seed_value;
    
    gCountA = gCountB = gCountC = 0;
    
    for (i = 0; i < SIZE; ++i) {
        selector = bad_hash(selector);
        simpleValueParent *newStruct = NULL;
        
        switch (process(selector)) {
            case 0:
                newStruct = (simpleValueParent *) makeSimpleValueA();
                ++gCountA;
                break;
            case 1:
                newStruct = (simpleValueParent *) makeSimpleValueB();
                ++gCountB;
                break;
            case 2:
                newStruct = (simpleValueParent *) makeSimpleValueC();
                ++gCountC;
                break;
            default:
                printf("How did we get a value not between 0 and 2?\n");
                exit(-1);
        }
        
        simpleValueData[i] = newStruct;
    }
}

/******************************************************************************/

void deleteSimpleValueData() {
    int i;
    for (i = 0; i < SIZE; ++i) {
        delete simpleValueData[i];
    }
}

/******************************************************************************/

void checkSimpleValueTypes() {
    int i, k;

    start_timer();
  
    for(i = 0; i < iterations; ++i) {
        int sumA, sumB, sumC;
        sumA = sumB = sumC = 0;
        for (k = 0; k < SIZE; ++k) {
            simpleValueParent *item = simpleValueData[k];
            typeValue typeData = item->type_value;
            
            switch( typeData ) {
                case typeValueA:
                    ++sumA;
                    break;
                case typeValueB:
                    ++sumB;
                    break;
                case typeValueC:
                    ++sumC;
                    break;
                default:
                    printf("How did we get a type of %d ?\n", typeData );
                    exit(-1);
            
            }
        }

        if (sumA != gCountA || sumB != gCountB || sumC != gCountC)
            printf("test %i type count failed\n", current_test);
    }
    
    record_result( timer(), "rtti simple value structs" );

}


/******************************************************************************/

void createSimpleStringData(reducer_func *process) {
    int i;
    unsigned int selector = seed_value;
    
    gCountA = gCountB = gCountC = 0;
    
    for (i = 0; i < SIZE; ++i) {
        selector = bad_hash(selector);
        simpleStringParent *newStruct = NULL;
        
        switch (process(selector)) {
            case 0:
                newStruct = (simpleStringParent *) makeSimpleStringA();
                ++gCountA;
                break;
            case 1:
                newStruct = (simpleStringParent *) makeSimpleStringB();
                ++gCountB;
                break;
            case 2:
                newStruct = (simpleStringParent *) makeSimpleStringC();
                ++gCountC;
                break;
            default:
                printf("How did we get a value not between 0 and 2?\n");
                exit(-1);
        }
        
        simpleStringData[i] = newStruct;
    }
}

/******************************************************************************/

void deleteSimpleStringData() {
    int i;
    for (i = 0; i < SIZE; ++i) {
        delete simpleStringData[i];
    }
}

/******************************************************************************/

void checkSimpleStringTypes() {
    int i, k;

    start_timer();
  
    for(i = 0; i < iterations; ++i) {
        int sumA, sumB, sumC;
        sumA = sumB = sumC = 0;
        for (k = 0; k < SIZE; ++k) {
            simpleStringParent *item = simpleStringData[k];
            const char *typeData = item->type_name;
            
            if (strcmp(typeData, "simpleStringA") == 0) {
                ++sumA;
            } else if (strcmp(typeData, "simpleStringB") == 0) {
                ++sumB;
            } else if (strcmp(typeData, "simpleStringC") == 0) {
                ++sumC;
            } else {
                printf("How did we get a type of %s ?\n", typeData );
                exit(-1);
            }
        }

        if (sumA != gCountA || sumB != gCountB || sumC != gCountC)
            printf("test %i type count failed\n", current_test);
    }
    
    record_result( timer(), "rtti simple string structs" );

}

/******************************************************************************/

void createClassData(reducer_func *process) {
    int i;
    unsigned int selector = seed_value;
    
    gCountA = gCountB = gCountC = 0;
    
    for (i = 0; i < SIZE; ++i) {
        selector = bad_hash(selector);
        parentClass *newStruct = NULL;
        
        switch (process(selector)) {
            case 0:
                newStruct = new subclassA();
                ++gCountA;
                break;
            case 1:
                newStruct = new subclassB();
                ++gCountB;
                break;
            case 2:
                newStruct = new subclassC();
                ++gCountC;
                break;


// if the classes can be created here, they can't be deadstripped
#if TEST_MORE_CLASSES

#define quickCase(X)    \
            case X: \
                newStruct = new subclassZ##X(); \
                break;

#define quickCaseTenPack(X)    \
    quickCase(1##X##0); \
    quickCase(1##X##1) \
    quickCase(1##X##2) \
    quickCase(1##X##3) \
    quickCase(1##X##4) \
    quickCase(1##X##5) \
    quickCase(1##X##6) \
    quickCase(1##X##7) \
    quickCase(1##X##8) \
    quickCase(1##X##9)
    
#define quickCaseHundredPack(K)    \
    quickCaseTenPack(K##0) \
    quickCaseTenPack(K##1) \
    quickCaseTenPack(K##2) \
    quickCaseTenPack(K##3) \
    quickCaseTenPack(K##4) \
    quickCaseTenPack(K##5) \
    quickCaseTenPack(K##6) \
    quickCaseTenPack(K##7) \
    quickCaseTenPack(K##8) \
    quickCaseTenPack(K##9)

#define quickCaseThousandPack(M)    \
    quickCaseHundredPack(M##0) \
    quickCaseHundredPack(M##1) \
    quickCaseHundredPack(M##2) \
    quickCaseHundredPack(M##3) \
    quickCaseHundredPack(M##4) \
    quickCaseHundredPack(M##5) \
    quickCaseHundredPack(M##6) \
    quickCaseHundredPack(M##7) \
    quickCaseHundredPack(M##8) \
    quickCaseHundredPack(M##9)

quickCaseThousandPack(0)
quickCaseThousandPack(1)

#endif    // TEST_MORE_CLASSES

            default:
                printf("How did we get a value not between 0 and 2?\n");
                exit(-1);
        }
        
        classData[i] = newStruct;
    }
}

/******************************************************************************/

void deleteClassData() {
    int i;
    for (i = 0; i < SIZE; ++i) {
        delete classData[i];
    }
}

/******************************************************************************/

void checkClassTypeNames() {
    int i, k;
    const char *typeNameA = typeid( subclassA ).name();
    const char *typeNameB = typeid( subclassB ).name();
    const char *typeNameC = typeid( subclassC ).name();

    start_timer();
  
    for(i = 0; i < iterations; ++i) {
        int sumA, sumB, sumC;
        sumA = sumB = sumC = 0;
        for (k = 0; k < SIZE; ++k) {
            parentClass *item = classData[k];
            const char *typeData = typeid( *item ).name();
            
            if (strcmp(typeData, typeNameA) == 0) {
                ++sumA;
            } else if (strcmp(typeData, typeNameB) == 0) {
                ++sumB;
            } else if (strcmp(typeData, typeNameC) == 0) {
                ++sumC;
            } else {
                printf("How did we get a type of %s ?\n", typeData );
                exit(-1);
            }
        }

        if (sumA != gCountA || sumB != gCountB || sumC != gCountC)
            printf("test %i type count failed\n", current_test);
    }
    
    record_result( timer(), "rtti class typeid names" );

}

/******************************************************************************/

void checkClassTypeInfo() {
    int i, k;

    start_timer();
    
    const std::type_info &infoA = typeid( subclassA );
    const std::type_info &infoB = typeid( subclassB );
    const std::type_info &infoC = typeid( subclassC );

    for(i = 0; i < iterations; ++i) {
        int sumA, sumB, sumC;
        sumA = sumB = sumC = 0;
        for (k = 0; k < SIZE; ++k) {
            parentClass *item = classData[k];

            const std::type_info &infoItem = typeid( *item );
            
            if (infoItem == infoA) {
                ++sumA;
            } else if (infoItem == infoB) {
                ++sumB;
            } else if (infoItem == infoC) {
                ++sumC;
            } else {
                printf("How did we get a type of %s ?\n", infoItem.name() );
                exit(-1);
            }
        }

        if (sumA != gCountA || sumB != gCountB || sumC != gCountC)
            printf("test %i type count failed\n", current_test);
    }
    
    record_result( timer(), "rtti class typeid type_info" );

}

/******************************************************************************/
/******************************************************************************/

int main(int argc, char** argv) {
  
    // output command for documentation:
    int i;
    for (i = 0; i < argc; ++i)
        printf("%s ", argv[i] );
    printf("\n");
    
#if TEST_MORE_CLASSES
    printf("With additional classes defined\n");
#endif

    if (argc > 1) iterations = atoi(argv[1]);
    if (argc > 2) init_value = (double) atof(argv[2]);
    if (argc > 3) seed_value = (unsigned int) atoi(argv[3]);

    createSimpleValueData(reduce_to_3);
    checkSimpleValueTypes();
    deleteSimpleValueData();

    createSimpleStringData(reduce_to_3);
    checkSimpleStringTypes();
    deleteSimpleStringData();

    createClassData(reduce_to_3);
    checkClassTypeInfo();
    checkClassTypeNames();
    deleteClassData();

    summarize("RTTI", SIZE, iterations, kDontShowGMeans, kDontShowPenalty);

    return 0;
}

// the end
/******************************************************************************/
/******************************************************************************/
