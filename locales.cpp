/*
    Copyright 2010 Adobe Systems Incorporated
    Copyright 2018 Chris Cox
    Distributed under the MIT License (see accompanying file LICENSE_1_0_0.txt
    or a copy at http://stlab.adobe.com/licenses.html )


Goal: Test the performance of locales, and streams using locales.


Assumptions:

    1) Repeatedly calling setlocale with the same locale should take near zero time.
    
    2) Calling setlocale with a NULL locale should take near zero time (returns pointer to constant data).
    
    3) localeconv should take near zero time (it is defined as returning a pointer to a constant struct).
    
    4) std::stream reading of ints should be no slower than using atol.
    
    5) std::stream reading of floats should be no slower than using atof.



NOTE: on Linux/Unix, use 'locale -a' to list the installed locales

NOTE: strtod and strtol ignore locales, and fail tests on any locale that uses a different decimal or comma character

*/

#include <cstdio>
#include <ctime>
#include <cmath>
#include <cstdlib>
#include <clocale>
#include <sstream>
#include <cinttypes>


// TODO - ccox - clean up the macro tests, separate into semi-sane groups
#if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
#define isBSD   1
#endif

// no longer available on Linux, not available on Windows
#if !defined(_LINUX_TYPES_H) && !defined(_SYS_TYPES_H) && !defined(_WIN32) && !defined(isBSD)
#include <xlocale.h>
#endif

#include "benchmark_stdint.hpp"
#include "benchmark_timer.h"
#include "benchmark_results.h"

/******************************************************************************/

int iterations = 20000;

#define SIZE     400

/******************************************************************************/

const char *LocaleStrings[] = {
    "",
    "C",
    "en_US",              // US English
    "en_US.ISO8859-1",
//    "en_US.ISO8859-15",        // !!!! fails on Linux
    "en_US.UTF-8",
    "de_DE",              // Deutsch / German
    "de_DE.UTF-8",
    "de_DE.ISO8859-1",
//    "ja_JP",            // Japanese -- !!!! fails on Linux, even after adding encoding
    "ja_JP.UTF-8",
    "tr_TR",              // Turkish
    "tr_TR.UTF-8",
    "cs_CZ.UTF-8",        // Czech
    "hr_HR.ISO8859-2",    // Croatian
  };

/******************************************************************************/
/******************************************************************************/

long global_integer_sum = 0;
int64_t global_64_sum = 0;
float global_float_sum = 0.0;
double global_double_sum = 0.0;

void CreateNumberStringStream( std::stringstream &integer_str, std::stringstream &float_str ) {
    int i;
    
    global_integer_sum = 0;
    global_64_sum = 0;
    for (i = 0; i < SIZE; ++i)
        {
        long value = i * 477;
        integer_str << value << " ";
        global_integer_sum += value;
        global_64_sum += (int64_t)value;
        }
    
    global_float_sum = 0.0;
    global_double_sum = 0.0;
    for (i = 0; i < SIZE; ++i)
        {
        double value = i * 47.3;
        float_str << value << " ";
        global_float_sum += (float)value;
        global_double_sum += value;
        }
}

/******************************************************************************/

inline void check_sum(long result) {
    if (result != global_integer_sum)
        printf("test %i failed (expect %ld, got %ld)\n", current_test, global_integer_sum, result);
}

/******************************************************************************/

inline void check_sum64(int64_t result) {
    if (result != global_64_sum)
        printf("test %i failed (expected %" PRIi64 ", got %" PRIi64 ")\n", current_test, global_64_sum, result);
}

/******************************************************************************/

inline void check_sum_float(float result) {
    float diff = fabs(result - global_float_sum);
    if ( diff > 0.1f )    // linux fabs or gcc makes a mess of the difference for some reason
        printf("test %i failed (expect %f, got %f, diff %f)\n", current_test, global_float_sum, result, diff);
}

/******************************************************************************/

inline void check_sum_double(double result) {
    double diff = fabs(result - global_double_sum);
    if ( diff > 0.01 )
        printf("test %i failed (expect %lf, got %lf, diff %f)\n", current_test, global_double_sum, result, diff);
}

/******************************************************************************/
/******************************************************************************/

void test_locale_basics()
{
    int i, j;
    
    
    // first verify that the setlocale function is working, and common locales are available
    for (i = 0; i < sizeof(LocaleStrings)/sizeof(LocaleStrings[0]); ++i)
        {
        if ( setlocale( LC_ALL, LocaleStrings[i] ) == NULL )
            {
            printf("Error: setlocale %s failed\n", LocaleStrings[i] );
            }
        }
    
    
    // time basic usage
    
    start_timer();
    for (i = 0; i != iterations; ++i)
        {
        for (j = 0; j < SIZE; ++j )
            {
            (void)setlocale( LC_ALL, "C" );
            }
        }
    record_result( timer(), "setlocale LC_ALL, C");
    
    
    start_timer();
    for (i = 0; i != iterations; ++i)
        {
        for (j = 0; j < SIZE; ++j )
            {
            (void)setlocale( LC_NUMERIC, "C" );
            }
        }
    record_result( timer(), "setlocale LC_NUMERIC, C");
    
    
    start_timer();
    for (i = 0; i != iterations; ++i)
        {
        for (j = 0; j < SIZE; ++j )
            {
            (void)setlocale( LC_TIME, "C" );
            }
        }
    record_result( timer(), "setlocale LC_TIME, C");
    
    start_timer();
    for (i = 0; i != iterations; ++i)
        {
        for (j = 0; j < SIZE; ++j )
            {
            (void)setlocale( LC_ALL, "" );
            }
        }
    record_result( timer(), "setlocale LC_ALL, empty");
    
    
    start_timer();
    for (i = 0; i != iterations; ++i)
        {
        for (j = 0; j < SIZE; ++j )
            {
            (void)setlocale( LC_NUMERIC, "" );
            }
        }
    record_result( timer(), "setlocale LC_NUMERIC, empty");
    
    
    start_timer();
    for (i = 0; i != iterations; ++i)
        {
        for (j = 0; j < SIZE; ++j )
            {
            (void)setlocale( LC_TIME, "" );
            }
        }
    record_result( timer(), "setlocale LC_TIME, empty");
    
    
    start_timer();
    for (i = 0; i != iterations; ++i)
        {
        for (j = 0; j < SIZE; ++j )
            {
            (void)setlocale( LC_ALL, "en_US" );
            }
        }
    record_result( timer(), "setlocale LC_ALL, en_US");
    
    
    start_timer();
    for (i = 0; i != iterations; ++i)
        {
        for (j = 0; j < SIZE; ++j )
            {
            (void)setlocale( LC_NUMERIC, "en_US" );
            }
        }
    record_result( timer(), "setlocale LC_NUMERIC, en_US");
    
    
    start_timer();
    for (i = 0; i != iterations; ++i)
        {
        for (j = 0; j < SIZE; ++j )
            {
            setlocale( LC_TIME, "en_US" );
            }
        }
    record_result( timer(), "setlocale LC_TIME, en_US");
    
    
    start_timer();
    for (i = 0; i != iterations; ++i)
        {
        for (j = 0; j < SIZE; ++j )
            {
            (void)setlocale( LC_ALL, "ja_JP.UTF-8" );
            }
        }
    record_result( timer(), "setlocale LC_ALL, ja_JP.UTF-8");
    
    
    start_timer();
    for (i = 0; i != iterations; ++i)
        {
        for (j = 0; j < SIZE; ++j )
            {
            (void)setlocale( LC_NUMERIC, "ja_JP.UTF-8" );
            }
        }
    record_result( timer(), "setlocale LC_NUMERIC, ja_JP.UTF-8");
    
    
    start_timer();
    for (i = 0; i != iterations; ++i)
        {
        for (j = 0; j < SIZE; ++j )
            {
            (void)setlocale( LC_TIME, "ja_JP.UTF-8" );
            }
        }
    record_result( timer(), "setlocale LC_TIME, ja_JP.UTF-8");
    
    start_timer();
    for (i = 0; i != iterations; ++i)
        {
        for (j = 0; j < SIZE; ++j )
            {
            (void)setlocale( LC_ALL, NULL );
            }
        }
    record_result( timer(), "setlocale LC_ALL, NULL");
    
    
    start_timer();
    for (i = 0; i != iterations; ++i)
        {
        for (j = 0; j < SIZE; ++j )
            {
            (void)setlocale( LC_NUMERIC, NULL );
            }
        }
    record_result( timer(), "setlocale LC_NUMERIC, NULL");
    
    
    start_timer();
    for (i = 0; i != iterations; ++i)
        {
        for (j = 0; j < SIZE; ++j )
            {
            (void)setlocale( LC_TIME, NULL );
            }
        }
    record_result( timer(), "setlocale LC_TIME, NULL");
    
    
    start_timer();
    for (i = 0; i != iterations; ++i)
        {
        volatile lconv *lcstruct;
        for (j = 0; j < SIZE; ++j )
            {
            lcstruct = localeconv();
            }
        }
    record_result( timer(), "localeconv");
    

// not available on Linux or Windows
#if !defined(_LINUX_TYPES_H) && !defined(_SYS_TYPES_H) && !defined(_WIN32) && !defined(isBSD)
    locale_t thisLocale = duplocale( LC_GLOBAL_LOCALE );    // Linux crashes without the duplicate
    start_timer();
    for (i = 0; i != iterations; ++i)
        {
        volatile lconv *lcstruct2;
        for (j = 0; j < SIZE; ++j )
            {
            lcstruct2 = localeconv_l( thisLocale );
            }
        }
    record_result( timer(), "localeconv_l");

    (void)freelocale( thisLocale );
#endif
    
    summarize("locales", SIZE, iterations, kDontShowGMeans, kDontShowPenalty );
    

    (void)setlocale( LC_ALL, "C" );
}

/******************************************************************************/

void test_locale_streams (const char *locale_to_test, const char *label = NULL)
{
    int i, j;
    const int kTempStringLimit = 200;
    
    if (label == NULL)
        label = locale_to_test;
    
    std::stringstream integer_str;
    std::stringstream float_str;
    
    // set a locale
    setlocale( LC_ALL, locale_to_test );

// not available on Windows
#if !defined(_WIN32) && !defined(isBSD)
    locale_t thisLocale = duplocale( LC_GLOBAL_LOCALE );    // Linux crashes without the duplicate
#endif
    
    // clear strings
    integer_str.str("");
    float_str.str("");
    
    // create strings with current locale formatting
    CreateNumberStringStream( integer_str, float_str );

    
    // and test reading ints and floats from the stream
    
    std::string readLongLabel( "stringstream read long " );
    readLongLabel += label;
    start_timer();
    for (i = 0; i != iterations; ++i)
        {
        integer_str.seekg( 0 );
        long sum = 0;
        for (j = 0; j < SIZE; ++j )
            {
            long temp;
            integer_str >> temp;
            sum += temp;
            }
        check_sum( sum );
        }
    record_result( timer(), readLongLabel.c_str() );
    
    std::string readFloatLabel( "stringstream read float " );
    readFloatLabel += label;
    start_timer();
    for (i = 0; i != iterations; ++i)
        {
        float_str.seekg( 0 );
        float sum = 0.0;
        for (j = 0; j < SIZE; ++j )
            {
            float temp;
            float_str >> temp;
            sum += temp;
            }
        check_sum_float( sum );
        }
    record_result( timer(), readFloatLabel.c_str() );
    
    std::string readDoubleLabel( "stringstream read double " );
    readDoubleLabel += label;
    start_timer();
    for (i = 0; i != iterations; ++i)
        {
        float_str.seekg( 0 );
        double sum = 0.0;
        for (j = 0; j < SIZE; ++j )
            {
            double temp;
            float_str >> temp;
            sum += temp;
            }
        check_sum_double( sum );
        }
    record_result( timer(), readDoubleLabel.c_str() );


    std::string readAtolLabel( "stringstream read atol " );
    readAtolLabel += label;
    start_timer();
    for (i = 0; i != iterations; ++i)
        {
        integer_str.seekg( 0 );
        long sum = 0;
        for (j = 0; j < SIZE; ++j )
            {
            char temp[kTempStringLimit];
            integer_str.getline( temp, kTempStringLimit, ' ' );
            sum += atol ( temp );
            }
        check_sum(sum);
        }
    record_result( timer(), readAtolLabel.c_str() );


    std::string readAtofLabel( "stringstream read atof " );
    readAtofLabel += label;
    start_timer();
    for (i = 0; i != iterations; ++i)
        {
        integer_str.seekg( 0 );
        double sum = 0;
        for (j = 0; j < SIZE; ++j )
            {
            char temp[kTempStringLimit];
            integer_str.getline( temp, kTempStringLimit, ' ' );
            sum += atof ( temp );
            }
        check_sum(sum);
        }
    record_result( timer(), readAtofLabel.c_str() );


// not available on Linux or Windows
#if !defined(_LINUX_TYPES_H) && !defined(_SYS_TYPES_H) && !defined(_WIN32) && !defined(isBSD)
    std::string readAtollLabel( "stringstream read atol_l " );
    readAtollLabel += label;
    start_timer();
    for (i = 0; i != iterations; ++i)
        {
        integer_str.seekg( 0 );
        long sum = 0;
        for (j = 0; j < SIZE; ++j )
            {
            char temp[kTempStringLimit];
            integer_str.getline( temp, kTempStringLimit, ' ' );
            sum += atol_l ( temp, thisLocale );
            }
        check_sum(sum);
        }
    record_result( timer(), readAtollLabel.c_str() );


    std::string readAtoflLabel( "stringstream read atof_l " );
    readAtoflLabel += label;
    start_timer();
    for (i = 0; i != iterations; ++i)
        {
        integer_str.seekg( 0 );
        double sum = 0;
        for (j = 0; j < SIZE; ++j )
            {
            char temp[kTempStringLimit];
            integer_str.getline( temp, kTempStringLimit, ' ' );
            sum += atof_l ( temp, thisLocale );
            }
        check_sum(sum);
        }
    record_result( timer(), readAtoflLabel.c_str() );
#endif


// not available on Windows or Solaris
#if !defined(_WIN32) && !defined(__sun) && !defined(isBSD)
    std::string readStrtodlLabel( "stringstream read strtod_l " );
    readStrtodlLabel += label;
    start_timer();
    for (i = 0; i != iterations; ++i)
        {
        integer_str.seekg( 0 );
        double sum = 0;
        for (j = 0; j < SIZE; ++j )
            {
            char temp[kTempStringLimit];
            integer_str.getline( temp, kTempStringLimit, ' ' );
            sum += strtod_l ( temp, NULL, thisLocale );
            }
        check_sum(sum);
        }
    record_result( timer(), readStrtodlLabel.c_str() );
#endif



    // reset locales, so our output doesn't change decimal characters :-)
#if !defined(_WIN32) && !defined(isBSD)
    (void)freelocale(thisLocale);
#endif

    (void)setlocale( LC_ALL, "C" );
    
    std::string summaryLabel( "stream locales " );
    summaryLabel += label;

    summarize( summaryLabel.c_str(), SIZE, iterations, kDontShowGMeans, kDontShowPenalty );

}

/******************************************************************************/

int main (int argc, char *argv[])
{
    
    // output command for documentation:
    int i;
    for (i = 0; i < argc; ++i)
        printf("%s ", argv[i] );
    printf("\n");

    if (argc > 1) iterations = atoi(argv[1]);
    
    test_locale_basics();
    
    test_locale_streams( "C" );
    test_locale_streams( "", "empty" );
    test_locale_streams( "en_US.UTF-8" );
    test_locale_streams( "de_DE.UTF-8" );
    test_locale_streams( "cs_CZ.UTF-8" );
    test_locale_streams( "ja_JP.UTF-8" );

    return 0;
}

/******************************************************************************/
/******************************************************************************/

