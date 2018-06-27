/*
    Copyright 2007-2008 Adobe Systems Incorporated
    Copyright 2018 Chris Cox
    Distributed under the MIT License (see accompanying file LICENSE_1_0_0.txt
    or a copy at http://stlab.adobe.com/licenses.html )


Goal: Examine performance when using C++ exceptions and compare to alternatives.

Assumptions:

    1) setjmp/longjmp have overhead when calling setjmp and when calling longjmp,
        due to saving registers, program counter, etc.
        Thus setjmp incurs a cost even when there is no error.

    2) Exceptions have very low overhead when no exception is thrown.
        There should be little or no cost for try/catch.

    3) Enabling exceptions adds little or no overhead to code paths that do not use exceptions.

    4) The time taken to throw an exception should be minimized.
        It should be on the order of longjmp plus freeing allocated objects.
        Exceptions may be rare, but they still need to return quickly.

**********

Exceptions are supposed to be rare events (significant errors, or other cases where prior work needs to be unwound).
Exceptions are not supposed to be used for flow control.

**********

compile options:

    -D TEST_WITH_EXCEPTIONS=1
        enables the C++ exceptions code
        without this flag, this source file is valid ANSI C and can be compiled without any exception handling
        This is to test for any compiler overhead when enabling exceptions, even if they are not used

*/

/******************************************************************************/

#include <stddef.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <setjmp.h>
#include "benchmark_results.h"
#include "benchmark_timer.h"

#ifndef TEST_WITH_EXCEPTIONS
#define TEST_WITH_EXCEPTIONS    0
#endif

/******************************************************************************/


/*  this constant may need to be adjusted to give reasonable minimum times */
/*  For best results, times should be about 1.0 seconds for the minimum test run */
/*  on 3Ghz desktop CPUs, 25k iterations is about 0.12 seconds */
int iterations = 25000;


/*  2000 items, or about 16k of data */
#define SIZE     2000


/*  initial value for filling our arrays, may be changed from the command line */
double init_value = 3.0;

/******************************************************************************/
/*  our global arrays of numbers to be summed */

double data[SIZE];

/******************************************************************************/

void check_sum(double result, const char *label) {
  if (result != SIZE * init_value) printf("test %s failed on sum\n", label);
}

void check_size(int size_count, const char *label) {
  if (size_count != SIZE) printf("test %s failed on size\n", label);
}

/******************************************************************************/

void fill( double *first, double *last, double value ) {
  while (first != last) *first++ = value;
}

/******************************************************************************/
/******************************************************************************/

/*  straightforward C style loops and addition */
/*  simple return code */

typedef int sum_return_type( double new_value, double *result );

int sum1_return( double new_value, double *result)
{
    *result += new_value;
    if (new_value < 0.0)
        return 1;
    return 0;
}

void test1(double* first, int count, sum_return_type *summer, const char *label) {
  int i;
  
  start_timer();
  
  for(i = 0; i < iterations; ++i) {
    double result = 0.0;
    int len = 0;
    int n;
    for (n = 0; n < count; ++n) {
        len += (summer)(first[n], &result );
    }
    if (len < SIZE) check_sum(result,label);
    if (first[0] < 0.0) check_size(len,label);
  }
  
  record_result( timer(), label );
}

/******************************************************************************/

int sum1_middle6(double new_value, double *result, sum_return_type *summer)
{
    *result -= 1.0;
    return (summer)( new_value, result );
}

int sum1_middle5(double new_value, double *result, sum_return_type *summer)
{
    int temp = sum1_middle6( new_value, result, summer );
    *result += 1.0;
    return temp;
}

int sum1_middle4(double new_value, double *result, sum_return_type *summer)
{
    *result -= 5.0;
    return sum1_middle5( new_value, result, summer );
}

int sum1_middle3(double new_value, double *result, sum_return_type *summer)
{
    int temp = sum1_middle4( new_value, result, summer );
    *result += 5.0;
    return temp;
}

int sum1_middle2(double new_value, double *result, sum_return_type *summer)
{
    *result -= 3.0;
    return sum1_middle3( new_value, result, summer );
}

int sum1_middle1(double new_value, double *result, sum_return_type *summer)
{
    int temp = sum1_middle2( new_value, result, summer );
    *result += 3.0;
    return temp;
}

void test1_deep(double* first, int count, sum_return_type *summer, const char *label) {
  int i;
  
  start_timer();
  
  for(i = 0; i < iterations; ++i) {
    double result = 0.0;
    int len = 0;
    int n;
    for (n = 0; n < count; ++n) {
        len += sum1_middle1( first[n], &result, summer );
    }
    if (len < SIZE) check_sum(result,label);
    if (first[0] < 0.0) check_size(len,label);
  }
  
  record_result( timer(), label );
}

/******************************************************************************/
/******************************************************************************/

/*  straightforward C style loops and addition */
/*  setjmp/longjmp */

typedef void sum1_longjmp_type( double new_value, double *result );

static jmp_buf *gLastJmp;

void sum1_longjmp( double new_value, double *result)
{
    *result += new_value;
    if (new_value < 0.0)
        longjmp(*gLastJmp, 1);
}

void test1_longjmp(double* first, int count, sum1_longjmp_type *summer, const char *label) {
  int i;
  
  start_timer();
  
  for(i = 0; i < iterations; ++i) {
    double result = 0.0;
    int len = 0;
    int n;
    for (n = 0; n < count; ++n) {
        jmp_buf    my_jump;
        
        if (setjmp( my_jump ) == 0) {
            gLastJmp = &my_jump;
            (summer)(first[n], &result );
        } else {
            ++len;
        }

    }
    if (len < SIZE) check_sum(result,label);
    if (first[0] < 0.0) check_size(len,label);
  }
  
  record_result( timer(), label );
}

/******************************************************************************/

void sum1_longjmp_middle6(double new_value, double *result, sum1_longjmp_type *summer)
{
    *result -= 1.0;
    (summer)( new_value, result );
}

void sum1_longjmp_middle5(double new_value, double *result, sum1_longjmp_type *summer)
{
    sum1_longjmp_middle6( new_value, result, summer );
    *result += 1.0;
}

void sum1_longjmp_middle4(double new_value, double *result, sum1_longjmp_type *summer)
{
    *result -= 5.0;
    sum1_longjmp_middle5( new_value, result, summer );
}

void sum1_longjmp_middle3(double new_value, double *result, sum1_longjmp_type *summer)
{
    sum1_longjmp_middle4( new_value, result, summer );
    *result += 5.0;
}

void sum1_longjmp_middle2(double new_value, double *result, sum1_longjmp_type *summer)
{
    *result -= 3.0;
    sum1_longjmp_middle3( new_value, result, summer );
}

void sum1_longjmp_middle1(double new_value, double *result, sum1_longjmp_type *summer)
{
    sum1_longjmp_middle2( new_value, result, summer );
    *result += 3.0;
}

void test1_longjmp_deep(double* first, int count, sum1_longjmp_type *summer, const char *label) {
  int i;
  
  start_timer();
  
  for(i = 0; i < iterations; ++i) {
    double result = 0.0;
    int len = 0;
    int n;
    for (n = 0; n < count; ++n) {
        jmp_buf    my_jump;
        
        if (setjmp( my_jump ) == 0) {
            gLastJmp = &my_jump;
            sum1_longjmp_middle1( first[n], &result, summer );
        } else {
            ++len;
        }
    }
    if (len < SIZE) check_sum(result,label);
    if (first[0] < 0.0) check_size(len,label);
  }
  
  record_result( timer(), label );
}

/******************************************************************************/
/******************************************************************************/

#if TEST_WITH_EXCEPTIONS

/*  straightforward C style loops and addition */
/*  using exception */

typedef void sum_exception_type( double new_value, double *result );


void sum1_exception( double new_value, double *result)
{
    *result += new_value;
    if (new_value < 0.0)
        throw 1;
}

void test1_exception(double* first, int count, sum_exception_type *summer, const char *label) {
  int i;
  
  start_timer();
  
  for(i = 0; i < iterations; ++i) {
    double result = 0.0;
    int len = 0;
    int n;
    for (n = 0; n < count; ++n) {
        try {
            (summer)(first[n], &result );
        }
        catch(int err) {
            ++len;
        }
    }
    if (len < SIZE) check_sum(result,label);
    if (first[0] < 0.0) check_size(len,label);
  }
  
  record_result( timer(), label );
}

/******************************************************************************/

void sum1_exception_middle6(double new_value, double *result, sum_exception_type *summer)
{
    *result -= 1.0;
    (summer)( new_value, result );
}

void sum1_exception_middle5(double new_value, double *result, sum_exception_type *summer)
{
    sum1_exception_middle6( new_value, result, summer );
    *result += 1.0;
}

void sum1_exception_middle4(double new_value, double *result, sum_exception_type *summer)
{
    *result -= 5.0;
    sum1_exception_middle5( new_value, result, summer );
}

void sum1_exception_middle3(double new_value, double *result, sum_exception_type *summer)
{
    sum1_exception_middle4( new_value, result, summer );
    *result += 5.0;
}

void sum1_exception_middle2(double new_value, double *result, sum_exception_type *summer)
{
    *result -= 3.0;
    sum1_exception_middle3( new_value, result, summer );
}

void sum1_exception_middle1(double new_value, double *result, sum_exception_type *summer)
{
    sum1_exception_middle2( new_value, result, summer );
    *result += 3.0;
}

void test1_exception_deep(double* first, int count, sum_exception_type *summer, const char *label) {
  int i;
  
  start_timer();
  
  for(i = 0; i < iterations; ++i) {
    double result = 0.0;
    int len = 0;
    int n;
    for (n = 0; n < count; ++n) {
        try {
            sum1_exception_middle1(first[n], &result, summer );
        }
        catch(int err) {
            ++len;
        }

    }
    if (len < SIZE) check_sum(result,label);
    if (first[0] < 0.0) check_size(len,label);
  }
  
  record_result( timer(), label );
}


#endif /*  TEST_WITH_EXCEPTIONS */


/******************************************************************************/
/******************************************************************************/

int main(int argc, char** argv) {
  
  /*  output command for documentation: */
  int i;
  for (i = 0; i < argc; ++i)
    printf("%s ", argv[i] );
  printf("\n");

  if (argc > 1) iterations = atoi(argv[1]);
  if (argc > 2) init_value = (double) atof(argv[2]);
  
  
  /*  fill with positive values */
  fill(data, data+SIZE, fabs(init_value));

#if TEST_WITH_EXCEPTIONS
  test1(data, SIZE, sum1_return, "simple return code not taken exceptions enabled");
  test1_deep(data, SIZE, sum1_return, "simple return code not taken deep exceptions enabled");
  test1_longjmp(data, SIZE, sum1_longjmp, "simple longjmp not taken exceptions enabled");
  test1_longjmp_deep(data, SIZE, sum1_longjmp, "simple longjmp not taken deep exceptions enabled");
  
  test1_exception(data, SIZE, sum1_exception, "simple exception not taken");
  test1_exception_deep(data, SIZE, sum1_exception, "simple exception not taken deep");
#else
  test1(data, SIZE, sum1_return, "simple return code not taken");
  test1_deep(data, SIZE, sum1_return, "simple return code not taken deep");
  test1_longjmp(data, SIZE, sum1_longjmp, "simple longjmp not taken");
  test1_longjmp_deep(data, SIZE, sum1_longjmp, "simple longjmp not taken deep");
#endif

  summarize("Exception Not Taken", SIZE, iterations, kDontShowGMeans, kDontShowPenalty );
  

  /*  fill with negative values */
  fill(data, data+SIZE, -fabs(init_value));
  
#if TEST_WITH_EXCEPTIONS
  test1(data, SIZE, sum1_return, "simple return code taken exceptions enabled");
  test1_deep(data, SIZE, sum1_return, "simple return code taken deep exceptions enabled");
  test1_longjmp(data, SIZE, sum1_longjmp, "simple longjmp taken exceptions enabled");
  test1_longjmp_deep(data, SIZE, sum1_longjmp, "simple longjmp taken deep exceptions enabled");
  
  test1_exception(data, SIZE, sum1_exception, "simple exception taken");
  test1_exception_deep(data, SIZE, sum1_exception, "simple exception taken deep");
#else
  test1(data, SIZE, sum1_return, "simple return code taken");
  test1_deep(data, SIZE, sum1_return, "simple return code taken deep");
  test1_longjmp(data, SIZE, sum1_longjmp, "simple longjmp taken");
  test1_longjmp_deep(data, SIZE, sum1_longjmp, "simple longjmp taken deep");
#endif
  
  summarize("Exception Taken", SIZE, iterations, kDontShowGMeans, kDontShowPenalty );


  return 0;
}

/*  the end */
/******************************************************************************/
/******************************************************************************/
