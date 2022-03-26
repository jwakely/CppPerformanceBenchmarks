/*
    Copyright 2009-2011 Adobe Systems Incorporated
    Copyright 2018-2022 Chris Cox
    Distributed under the MIT License (see accompanying file LICENSE_1_0_0.txt
    or a copy at http://stlab.adobe.com/licenses.html )


Goal: Test the performance of various common ways of parsing a number from a string.

Assumptions:
    
    1) atol, atof, strtol, strtof, etc. should all be faster than scanf, due to lower overhead
    
    2) strtof, strtol should be simlar in performance to atof, atol
    
    3) strtol and strtoul should have similar performance
    
    4) strtof and strtod should have similar performance
    
    5) std::stoi,stol,stoll,stoul,stoull and std::stof,stod all have additional overhead from using std:string
        which usually makes them a bit slower than stol,strtol, etc.
    
    5) library routines should be the same speed or faster than simple source versions of the same functions



Don't forget that numbers and hex values are used often when writing HTML, XML, and JSON data.
    (and far too often in ASCII based 3D file formats)



TODO - std::from_chars
    C++17, not completely implemented under LLVM or GCC
    GCC 11.2.1 appears to be working (SuseTW), but not yet available in several Linux distributions

TODO - check https://github.com/abseil/abseil-cpp
TODO - check https://lemire.me/blog/2021/01/29/number-parsing-at-a-gigabyte-per-second/       https://arxiv.org/pdf/2101.11408.pdf

TODO - test speed of octal and binary parsing?  Are they really used commonly anywhere (outside of XKCD)?

TODO - test speed of error cases?

TODO - unit test error handling?


*/

#include <cstdio>
#include <ctime>
#include <climits>
#include <cerrno>
#include <cinttypes>
#include <cstdlib>
#include <cmath>
#include <string>
#include <utility>
#include <charconv>
#include "benchmark_stdint.hpp"
#include "benchmark_timer.h"
#include "benchmark_results.h"

/******************************************************************************/

int iterations = 40000;

#define SIZE     1200

const int max_number_size = 50; // some unit test strings are longer, for binary

/******************************************************************************/

typedef char test_type[ SIZE ][ max_number_size ];
test_type integer_strings;
test_type hex_strings;
test_type float_strings;
test_type float_stringsE;

/******************************************************************************/

long global_integer_sum = 0;
unsigned long global_uinteger_sum = 0;
int64_t global_64_sum = 0;
uint64_t global_u64_sum = 0;
float global_float_sum = 0.0;
double global_double_sum = 0.0;

/******************************************************************************/

static void printStringStats(const char *type, test_type strings)
{
    size_t total_chars = 0;
    size_t min_chars = max_number_size*4;
    size_t max_chars = 0;
    for (long i = 0; i < SIZE; ++i)
        {
        size_t len = strlen( strings[i] );
        total_chars += len;
        min_chars = std::min( len, min_chars );
        max_chars = std::max( len, max_chars );
        }
    printf("average chars %s = %lu [ %lu ... %lu ]\n", type, total_chars/SIZE, min_chars, max_chars );
}

/******************************************************************************/

void CreateIntegerStrings() {
    
    global_integer_sum = 0;
    global_uinteger_sum = 0;
    global_64_sum = 0;
    global_u64_sum = 0;
    
    for (long i = 0; i < SIZE; ++i)
        {
        long value_int;
        
        if (i < 75)
            value_int = i;
        else
            {
            double x = double(i) / (SIZE);
            double x2 = pow( x, 6.191 );
            value_int = long(INT_MAX * x2 + 0.5);   // keep values inside 32 bit range for now, so we can test more APIs
            }
        
        //printf("%ld, 0x%lX\n", value_int, value_int );      // %.32e
        
        snprintf( integer_strings[i], max_number_size-1, "%ld", value_int );
        snprintf( hex_strings[i], max_number_size-1, "0x%lX", value_int );
        
        global_uinteger_sum += (unsigned long)value_int;
        global_integer_sum += (long)value_int;
        global_u64_sum += (uint64_t)value_int;
        global_64_sum += (int64_t)value_int;
        }
        

#if PRINT_SOME_STATS
/*
limit to 32 bit integers and floats because of some APIs

average chars integer = 7 [ 1 ... 10 ]
average chars hex = 8 [ 3 ... 10 ]
*/
    printStringStats( "integer", integer_strings );
    printStringStats( "hex", hex_strings );
#endif

}

/******************************************************************************/

void CreateFloatStrings() {
    
    global_float_sum = 0.0;
    global_double_sum = 0.0;

/*
#   define    HUGE_VAL     1e500
#   define    HUGE_VALF    1e50f
#   define    HUGE_VALL    1e5000L
 */
    const double maxFloatVal = 1e19; // keep values well inside float range for now, so we can test more APIs
    
    for (long i = 0; i < SIZE; ++i)
        {
        double value;
        
        if (i < 75)
            value = i;
        else
            {
            double x = double(i) / (SIZE);
            double x2 = pow( x, 14.191 );
            value = maxFloatVal * x2;
            }
        
        //printf("%f, %.19e\n", value, value );      // %.32e
        
        snprintf( float_strings[i], max_number_size-1, "%f", value );
        snprintf( float_stringsE[i], max_number_size-1, "%.19e", value );
        global_float_sum += float(value);
        global_double_sum += double(value);
        }
        
        
#if PRINT_SOME_STATS
/*
limit to 32 bit integers and floats because of some APIs

average chars float = 20 [ 8 ... 26 ]
average chars sci = 25 [ 25 ... 25 ]
*/

    printStringStats( "float", float_strings );
    printStringStats( "sci", float_stringsE );
#endif

}

/******************************************************************************/

void CreateNumberStrings() {

    CreateIntegerStrings();
    CreateFloatStrings();

}

/******************************************************************************/
/******************************************************************************/

inline void check_sum(long result) {
    if (result != global_integer_sum)
        printf("test %i failed (%ld, %lu)\n", current_test, result, global_uinteger_sum);
}

/******************************************************************************/

inline void check_sum(unsigned long result) {
    if (result != global_uinteger_sum)
        printf("test %i failed (%lu, %lu)\n", current_test, result, global_uinteger_sum);
}

/******************************************************************************/

inline void check_sum64(int64_t result) {
    if (result != global_64_sum)
        printf("test %i failed (%" PRIi64 ", %" PRIu64 ")\n", current_test, result, global_64_sum);
}

/******************************************************************************/

inline void check_sum64(uint64_t result) {
    if (result != global_u64_sum)
        printf("test %i failed (%" PRIu64 ", %" PRIu64 ")\n", current_test, result, global_u64_sum);
}

/******************************************************************************/

// some implementations are < 0.04, while some are off by 0.16+
inline void check_sum_float(float result) {
    if (fabs(result - global_float_sum) > 0.17)
        printf("test %i failed (%f, %f)\n", current_test, result, global_float_sum);
}

/******************************************************************************/

inline void check_sum_double(double result) {
    if (fabs(result - global_double_sum) > 0.05)
        printf("test %i failed (%f, %f)\n", current_test, result, global_double_sum);
}

/******************************************************************************/
/******************************************************************************/


inline bool quick_isspace( int value )
    { return (value == ' '); }

inline bool quick_isdigit( int value )
    { return ((value <= '9') && (value >= '0')); }

//inline bool quick_isoctal( int value )
//    { return ((value <= '7') && (value >= '0')); }

//inline bool quick_isxdigit( int value )
//    { return ( ((value <= '9') && (value >= '0')) || ((value <= 'f') && (value >='a')) || ((value <= 'F') && (value >='A')) ); }

// the alternative is a table of 380 entries to decode the exponent directly (3040 bytes)
const static double powersOf10[] = {    1.0e1,
                                        1.0e2,
                                        1.0e4,
                                        1.0e8,
                                        1.0e16,
                                        1.0e32,
                                        1.0e64,
                                        1.0e128,
                                        1.0e256
                                    };


// this could also be constructed in place, but requires double->mem->int->mem->double conversions that are SLOW on x86
const size_t fraction_digit_limit = 32;
const static double fraction_multiplier[ fraction_digit_limit ] = {   // usually only 6 to 16 digits
                                        1.0e0,
                                        1.0e-1,
                                        1.0e-2,
                                        1.0e-3,
                                        1.0e-4,
                                        1.0e-5,
                                        1.0e-6,
                                        1.0e-7,
                                        1.0e-8,
                                        1.0e-9,
                                        1.0e-10,
                                        1.0e-11,
                                        1.0e-12,
                                        1.0e-13,
                                        1.0e-14,
                                        1.0e-15,
                                        1.0e-16,
                                        1.0e-17,
                                        1.0e-18,
                                        1.0e-19,
                                        1.0e-20,
                                        1.0e-21,
                                        1.0e-22,
                                        1.0e-23,
                                        1.0e-24,
                                        1.0e-25,
                                        1.0e-26,
                                        1.0e-27,
                                        1.0e-28,
                                        1.0e-29,
                                        1.0e-30,
                                        1.0e-31,
                                    };

double
simple_strtod(const char *nptr, char **endptr)
{
    double result = 0.0;
    bool sign_negative = false;
    const char *lastp = nptr;
    const char *p = nptr;

    while (quick_isspace(*p))
        p++;

    // handle optional signs
    if (*p == '-') {
        sign_negative = true;
        ++p;
    }
    else if (*p == '+')
        ++p;


// TODO - handle hex notation? See math.h
//        But BSD and other open sources I can find don't appear to implement hex!?
// 0x1.fffffep+127f
// 0xb.17217f7d1cf79acp-4L
// printf %A %a

    
    // special cases INFINITY, NAN, or possible error case
    if (!quick_isdigit(*p)) {
    
        if (*p == 'I' || *p == 'i') {
            if ((strcmp(p+1,"NFINITY") == 0) || (strcmp(p+1,"nfinity") == 0)
                || (strcmp(p+1,"NF") == 0) || (strcmp(p+1,"nf") == 0)) {
                result = INFINITY;
                lastp = p+8;
                goto done;
            }
        }
        else if (*p == 'N' || *p == 'n') {
            if ((p[1] == 'a' || p[1] == 'A') && (p[2] == 'N' || p[2] == 'n') ) {
                result = NAN;
                lastp = p+3;
                goto done;
            }
        }

        // unknown non-numeric data
        goto done;
    }


    {
    // integer significand
    // potentially large number of digits
    uint64_t int_result = (*p++ - '0');

    while (quick_isdigit(p[0]) && quick_isdigit(p[1])) {
        int_result = int_result * 100;
        uint32_t digit0 = p[0] - '0';
        uint32_t digit1 = p[1] - '0';
        int_result += 10*digit0 + digit1;
        p += 2;
    }

    while (quick_isdigit(p[0])) {
        int_result = int_result * 10;
        int_result += p[0] - '0';
        ++p;
    }

// TODO - overrange detection?
    
    result = double(int_result);
    lastp = p;
    }


    // decimal and fraction
    if (*p == '.') {
        double fraction = 0.0;
        ++p;

        uint64_t int_fraction = 0;
        auto start_p = p;
        
        // potentially large number of digits
        while (quick_isdigit(p[0]) && quick_isdigit(p[1])) {
            int_fraction *= 100;
            uint32_t digit0 = p[0] - '0';
            uint32_t digit1 = p[1] - '0';
            int_fraction += 10*digit0 + digit1;
            p += 2;
        }
        
        while (quick_isdigit(*p)) {
            int_fraction *= 10;
            int_fraction += (*p - '0');
            ++p;
        }

// TODO - overrange detection?
        
        uint32_t fraction_digits = (p - start_p);
        if (fraction_digits > 0)
            fraction = double(int_fraction) * fraction_multiplier[ fraction_digits ];
        
        result += fraction;
        lastp = p;
    }
    
    
    if (sign_negative)
        result = -result;


    // handle optional exponent notation
    if ((*p == 'E') || (*p == 'e')) {
        int e = 0;
        bool exp_negative = false;
        
        ++p;

        if (*p == '-') {
            exp_negative = true;
            ++p;
        }
        else if (*p == '+')
            ++p;

        if (quick_isdigit(*p)) {
            // do not skip zeros here!
            
            // should be maximum of 3 digits, could unroll
            e = (int)(*p++ - '0');
            while (quick_isdigit(*p)) {
                e = e * 10 + (int)(*p - '0');
                ++p;
            }
            lastp = p;

            // float exp max = +-38
            // double exp max = +-308
            if (e > 308) {
                result = 0.0;
                errno = ERANGE;
                goto done;
            }

            if (e != 0) {
                double exponent = 1.0;
                
                // this could be unrolled - but so far it doesn't change the speed
                // limiting range seems to help more

                if (e < 8)
                    for (int j = 0; j < 3; ++j) {    // max 308 (9 bits) for double, max 38 (6 bits) for float
                        if ((e & (1 << j)) != 0)
                            exponent *= powersOf10[j];
                    }
                else if (e < 64)
                    for (int j = 0; j < 6; ++j) {    // max 308 (9 bits) for double, max 38 (6 bits) for float
                        if ((e & (1 << j)) != 0)
                            exponent *= powersOf10[j];
                    }
                else
                    for (int j = 0; j < 9; ++j) {    // max 308 (9 bits) for double, max 38 (6 bits) for float
                        if ((e & (1 << j)) != 0)
                            exponent *= powersOf10[j];
                    }
     
                if (exp_negative)
                    result /= exponent;
                else
                    result *= exponent;
            }
        }
        else if (!quick_isdigit(*(lastp-1))) {
            lastp = nptr;
            //goto done;
        }
    
    }
    else if (p > nptr && !quick_isdigit(*(p-1))) {    // did we end on a bad character?
        lastp = nptr;
        //goto done;
    }


done:
    if (endptr)
        *endptr = (char*)lastp;
    
    return result;
}

/******************************************************************************/

int64_t
simple_strtol(const char *str, char **endptr, int base)
{
    int64_t result = 0;
    bool sign_negative = false;
    const char *lastp = str;
    const char *p = str;
    
    static int64_t division_table[ 2*37 ];       // index 0,1 are never used, but cleaner to index directly  A..Z = 10..36
    static unsigned char hex_table[ 256 ];       // actually used for all uncommon bases
    static bool tables_initialized = false;
    
    
    if ( base != 0 && (base < 2 || base > 36) )
        return 0;
    
    // setup cached tables - could also be hard coded, or defined by locale
    if (!tables_initialized) {
        memset(hex_table,255,sizeof(hex_table));
        memset(division_table,0,sizeof(division_table));
        for ( long i = '0'; i <= '9'; ++i)
            hex_table[i] = i - '0';
        for ( long j = 'A'; j <= 'Z'; ++j)
            hex_table[j] = 10 + j - 'A';
        for ( long k = 'a'; k <= 'z'; ++k)
            hex_table[k] = 10 + k - 'a';
        tables_initialized = true;
    }

    while (quick_isspace(*p))
        p++;
    
    if (*p == '-') {
        sign_negative = true;
        ++p;
    }
    else if (*p == '+')
        ++p;
    
    
    if ((base == 0 || base == 16) && (p[0] == '0')) {
        if (p[1] == 'x' || p[1] == 'X') {
            p += 2;
            base = 16;
        }
        else if (base == 0) {    // octal special case
            p++;
            base = 8;
        }
    }
    
    if (base == 0)    // special case base not set, but did not see a special prefix, so becomes base 10
        base = 10;

    
    if (base == 10) {    // this is faster than the generic loop below, offering more optimization opportunities

        if (quick_isdigit(*p)) {
            const int64_t upper_limit = LLONG_MAX/10;
            const int64_t upper_limit100 = LLONG_MAX/100;
            result = (long)(*p++ - '0');

            while (quick_isdigit(p[0]) && quick_isdigit(p[1])) {
                long digit0 = (long)(p[0] - '0');
                long digit1 = (long)(p[1] - '0');
                if (result > upper_limit100) { goto error; } // if this passes, then any 2 digits added are still less than LONG_MAX
                result *= 100;
                result += 10*digit0 + digit1;  // would  LUT be faster?
                p += 2;
            }

            while (quick_isdigit(p[0])) {
                long digit = (long)(p[0] - '0');
                if (result > upper_limit) { goto error; } // if this passes, then any digit added is still less than LONG_MAX
                result *= 10;
                result += digit;
                ++p;
            }
            lastp = p;
        }

    } else if (base == 16) {    // this is faster than the generic base version below
        
        const int64_t upper_limit = LLONG_MAX / 16;
        const int64_t upper_limit2 = LLONG_MAX / 256;
        
        while (p[0] && p[1]) {
            long digit0 = hex_table[p[0]];
            long digit1 = hex_table[p[1]];
            if (result > upper_limit2) { goto error; } // if this passes, then any 2 digits added are still less than LONG_MAX
            if ((digit0 >= 16) || (digit1 >= 16)) break;
            result <<= 8;
            result += (digit0<<4) + digit1;
            p += 2;
        }
        
        while (*p) {
            long digit = hex_table[*p];
            if (result > upper_limit) { goto error; } // if this passes, then any digit added is still less than LONG_MAX
            if (digit >= 16) break;
            result <<= 4;
            result += digit;
            ++p;
        }
        lastp = p;
    } else {
        // generic base code
        
        if (division_table[2*base] == 0) {
            division_table[2*base+0] = LLONG_MAX / base;
            division_table[2*base+1] = LLONG_MAX / (base*base);
        }
        
        const int64_t base2 = base*base;
        const int64_t upper_limit = division_table[2*base+0];
        const int64_t upper_limit2 = division_table[2*base+1];
        
        while (p[0] && p[1]) {
            long digit0 = hex_table[p[0]];
            long digit1 = hex_table[p[1]];
            if (result > upper_limit2) { goto error; } // if this passes, then any 2 digits added are still less than LONG_MAX
            if ((digit0 >= base) || (digit1 >= base)) break;
            result *= base2;
            result += base*digit0 + digit1;  // would  LUT be faster?
            p += 2;
        }

        while (*p) {
            long digit = hex_table[*p];
            if (result > upper_limit) { goto error; } // if this passes, then any digit added is still less than LONG_MAX
            if (digit >= base) break;
            result *= base;
            result += digit;
            ++p;
        }
        lastp = p;
    }

    if (endptr)
        *endptr = (char*)lastp;
    
    return ((sign_negative) ? -result : result);


error:
    errno = ERANGE;
    if (endptr)
        *endptr = (char*)p;
    return LLONG_MAX;
}

/******************************************************************************/
/******************************************************************************/

void testInteger()
{
    int i, j;

    start_timer();
    for (i = 0; i != iterations; ++i)
        {
        long sum = 0;
        for (j = 0; j < SIZE; ++j )
            {
            sum += atol ( integer_strings[j] );
            }
        check_sum(sum);
        }
    record_result( timer(), "atol");
    
    
    start_timer();
    for (i = 0; i != iterations; ++i)
        {
        long sum = 0;
        for (j = 0; j < SIZE; ++j )
            {
            sum += atoi ( integer_strings[j] );
            }
        check_sum(sum);
        }
    record_result( timer(), "atoi");
    
    
    start_timer();
    for (i = 0; i != iterations; ++i)
        {
        long sum = 0;
        for (j = 0; j < SIZE; ++j )
            {
            sum += strtol ( integer_strings[j], NULL, 0 );
            }
        check_sum(sum);
        }
    record_result( timer(), "strtol");
    
    
    start_timer();
    for (i = 0; i != iterations; ++i)
        {
        unsigned long sum = 0;
        for (j = 0; j < SIZE; ++j )
            {
            sum += strtoul ( integer_strings[j], NULL, 0 );
            }
        check_sum(sum);
        }
    record_result( timer(), "strtoul");
    
    
    start_timer();
    for (i = 0; i != iterations; ++i)
        {
        long sum = 0;
        for (j = 0; j < SIZE; ++j )
            {
            int result;
            sscanf( integer_strings[j], "%d", &result );
            sum += result;
            }
        check_sum(sum);
        }
    record_result( timer(), "sscanf d");
    
    
    start_timer();
    for (i = 0; i != iterations; ++i)
        {
        int64_t sum = 0;
        for (j = 0; j < SIZE; ++j )
            {
            sum += atoll ( integer_strings[j] );
            }
        check_sum64(sum);
        }
    record_result( timer(), "atoll");
    

    start_timer();
    for (i = 0; i != iterations; ++i)
        {
        int64_t sum = 0;
        for (j = 0; j < SIZE; ++j )
            {
            sum += strtoll ( integer_strings[j], NULL, 0 );
            }
        check_sum64(sum);
        }
    record_result( timer(), "strtoll");


    start_timer();
    for (i = 0; i != iterations; ++i)
        {
        uint64_t sum = 0;
        for (j = 0; j < SIZE; ++j )
            {
            sum += strtoull ( integer_strings[j], NULL, 0 );
            }
        check_sum64(sum);
        }
    record_result( timer(), "strtoull");
    
    
    start_timer();
    for (i = 0; i != iterations; ++i)
        {
        int64_t sum = 0;
        for (j = 0; j < SIZE; ++j )
            {
            int64_t result;
            sscanf( integer_strings[j], "%" SCNi64, &result );
            sum += result;
            }
        check_sum64(sum);
        }
    record_result( timer(), "sscanf ll");
    

    start_timer();
    for (i = 0; i != iterations; ++i)
        {
        long sum = 0;
        for (j = 0; j < SIZE; ++j )
            {
            std::string temp( integer_strings[j] );
            sum += std::stoi (temp);
            }
        check_sum(sum);
        }
    record_result( timer(), "std::stoi");
    

    start_timer();
    for (i = 0; i != iterations; ++i)
        {
        long sum = 0;
        for (j = 0; j < SIZE; ++j )
            {
            std::string temp( integer_strings[j] );
            sum += std::stol (temp);
            }
        check_sum(sum);
        }
    record_result( timer(), "std::stol");
    

    start_timer();
    for (i = 0; i != iterations; ++i)
        {
        uint64_t sum = 0;
        for (j = 0; j < SIZE; ++j )
            {
            std::string temp( integer_strings[j] );
            sum += std::stoul (temp);
            }
        check_sum64(sum);
        }
    record_result( timer(), "std::stoul");
    

    start_timer();
    for (i = 0; i != iterations; ++i)
        {
        int64_t sum = 0;
        for (j = 0; j < SIZE; ++j )
            {
            std::string temp( integer_strings[j] );
            sum += std::stoll (temp);
            }
        check_sum64(sum);
        }
    record_result( timer(), "std::stoll");
    

    start_timer();
    for (i = 0; i != iterations; ++i)
        {
        uint64_t sum = 0;
        for (j = 0; j < SIZE; ++j )
            {
            std::string temp( integer_strings[j] );
            sum += std::stoull (temp);
            }
        check_sum64(sum);
        }
    record_result( timer(), "std::stoull");


#if __cplusplus >= 201703
    start_timer();
    for (i = 0; i != iterations; ++i)
        {
        int64_t sum = 0;
        for (j = 0; j < SIZE; ++j )
            {
            int64_t value;
            (void)std::from_chars( integer_strings[j], integer_strings[j]+max_number_size, value );
            sum += value;
            }
        check_sum64(sum);
        }
    record_result( timer(), "std::from_chars");
#endif

    
    start_timer();
    for (i = 0; i != iterations; ++i)
        {
        long sum = 0;
        for (j = 0; j < SIZE; ++j )
            {
            sum += simple_strtol ( integer_strings[j], NULL, 0 );
            }
        check_sum(sum);
        }
    record_result( timer(), "simple_strtol");
    
    
    summarize("atol", SIZE, iterations, kDontShowGMeans, kDontShowPenalty );
}

/******************************************************************************/

void testHex()
{
    int i, j;

    start_timer();
    for (i = 0; i != iterations; ++i)
        {
        long sum = 0;
        for (j = 0; j < SIZE; ++j )
            {
            sum += strtol ( hex_strings[j], NULL, 16 );
            }
        check_sum(sum);
        }
    record_result( timer(), "strtol hex");
    
    
    start_timer();
    for (i = 0; i != iterations; ++i)
        {
        unsigned long sum = 0;
        for (j = 0; j < SIZE; ++j )
            {
            sum += strtoul ( hex_strings[j], NULL, 16 );
            }
        check_sum(sum);
        }
    record_result( timer(), "strtoul hex");
    
    
    start_timer();
    for (i = 0; i != iterations; ++i)
        {
        long sum = 0;
        for (j = 0; j < SIZE; ++j )
            {
            int result;
            sscanf( hex_strings[j], "%X", &result );
            sum += result;
            }
        check_sum(sum);
        }
    record_result( timer(), "sscanf X");


    start_timer();
    for (i = 0; i != iterations; ++i)
        {
        int64_t sum = 0;
        for (j = 0; j < SIZE; ++j )
            {
            sum += strtoll ( hex_strings[j], NULL, 16 );
            }
        check_sum64(sum);
        }
    record_result( timer(), "strtoll hex");


    start_timer();
    for (i = 0; i != iterations; ++i)
        {
        uint64_t sum = 0;
        for (j = 0; j < SIZE; ++j )
            {
            sum += strtoull ( hex_strings[j], NULL, 16 );
            }
        check_sum64(sum);
        }
    record_result( timer(), "strtoull hex");
    
    
    start_timer();
    for (i = 0; i != iterations; ++i)
        {
        uint64_t sum = 0;
        for (j = 0; j < SIZE; ++j )
            {
            uint64_t result;
            sscanf( hex_strings[j], "%" SCNx64, &result );
            sum += result;
            }
        check_sum64(sum);
        }
    record_result( timer(), "sscanf llX");
    

    start_timer();
    for (i = 0; i != iterations; ++i)
        {
        long sum = 0;
        for (j = 0; j < SIZE; ++j )
            {
            std::string temp( hex_strings[j] );
            sum += std::stoi (temp, NULL, 16);
            }
        check_sum(sum);
        }
    record_result( timer(), "std::stoi hex");
    

    start_timer();
    for (i = 0; i != iterations; ++i)
        {
        long sum = 0;
        for (j = 0; j < SIZE; ++j )
            {
            std::string temp( hex_strings[j] );
            sum += std::stol (temp, NULL, 16);
            }
        check_sum(sum);
        }
    record_result( timer(), "std::stol hex");
    

    start_timer();
    for (i = 0; i != iterations; ++i)
        {
        uint64_t sum = 0;
        for (j = 0; j < SIZE; ++j )
            {
            std::string temp( hex_strings[j] );
            sum += std::stoul (temp, NULL, 16);
            }
        check_sum64(sum);
        }
    record_result( timer(), "std::stoul hex");
    

    start_timer();
    for (i = 0; i != iterations; ++i)
        {
        int64_t sum = 0;
        for (j = 0; j < SIZE; ++j )
            {
            std::string temp( hex_strings[j] );
            sum += std::stoll (temp, NULL, 16);
            }
        check_sum64(sum);
        }
    record_result( timer(), "std::stoll hex");
    

    start_timer();
    for (i = 0; i != iterations; ++i)
        {
        uint64_t sum = 0;
        for (j = 0; j < SIZE; ++j )
            {
            std::string temp( hex_strings[j] );
            sum += std::stoull (temp, NULL, 16);
            }
        check_sum64(sum);
        }
    record_result( timer(), "std::stoull hex");


#if __cplusplus >= 201703
// cannot handle 0x prefix
    start_timer();
    for (i = 0; i != iterations; ++i)
        {
        uint64_t sum = 0;
        for (j = 0; j < SIZE; ++j )
            {
            uint64_t value;
            (void)std::from_chars( hex_strings[j]+2, hex_strings[j]+max_number_size, value, 16 );
            sum += value;
            }
        check_sum64(sum);
        }
    record_result( timer(), "std::from_chars hex");
#endif

    
    start_timer();
    for (i = 0; i != iterations; ++i)
        {
        long sum = 0;
        for (j = 0; j < SIZE; ++j )
            {
            sum += simple_strtol ( hex_strings[j], NULL, 16 );
            }
        check_sum(sum);
        }
    record_result( timer(), "simple_strtol hex");
    
    summarize("atol hex", SIZE, iterations, kDontShowGMeans, kDontShowPenalty );
}

/******************************************************************************/

void testFloat()
{
    int i, j;

    start_timer();
    for (i = 0; i != iterations; ++i)
        {
        double sum = 0.0;
        for (j = 0; j < SIZE; ++j )
            {
            sum += atof ( float_strings[j] );
            }
        check_sum_double(sum);
        }
    record_result( timer(), "atof");
    
    
    start_timer();
    for (i = 0; i != iterations; ++i)
        {
        float sum = 0.0;
        for (j = 0; j < SIZE; ++j )
            {
            sum += strtof ( float_strings[j], NULL );
            }
        check_sum_float(sum);
        }
    record_result( timer(), "strtof");


    start_timer();
    for (i = 0; i != iterations; ++i)
        {
        double sum = 0.0;
        for (j = 0; j < SIZE; ++j )
            {
            sum += strtod ( float_strings[j], NULL );
            }
        check_sum_double(sum);
        }
    record_result( timer(), "strtod");
    
    
    start_timer();
    for (i = 0; i != iterations; ++i)
        {
        float sum = 0.0;
        for (j = 0; j < SIZE; ++j )
            {
            float result;
            sscanf( float_strings[j], "%f", &result );
            sum += result;
            }
        check_sum_float(sum);
        }
    record_result( timer(), "sscanf f float");
    
    
    start_timer();
    for (i = 0; i != iterations; ++i)
        {
        float sum = 0.0;
        for (j = 0; j < SIZE; ++j )
            {
            float result;
            sscanf( float_strings[j], "%g", &result );
            sum += result;
            }
        check_sum_float(sum);
        }
    record_result( timer(), "sscanf g float");
    
    
    start_timer();
    for (i = 0; i != iterations; ++i)
        {
        double sum = 0.0;
        for (j = 0; j < SIZE; ++j )
            {
            double result;
            sscanf( float_strings[j], "%lf", &result );
            sum += result;
            }
        check_sum_double(sum);
        }
    record_result( timer(), "sscanf f double");
    
    
    start_timer();
    for (i = 0; i != iterations; ++i)
        {
        double sum = 0.0;
        for (j = 0; j < SIZE; ++j )
            {
            double result;
            sscanf( float_strings[j], "%lg", &result );
            sum += result;
            }
        check_sum_double(sum);
        }
    record_result( timer(), "sscanf g double");
    

    start_timer();
    for (i = 0; i != iterations; ++i)
        {
        float sum = 0;
        for (j = 0; j < SIZE; ++j )
            {
            std::string temp( float_strings[j] );
            sum += std::stof (temp);
            }
        check_sum_float(sum);
        }
    record_result( timer(), "std::stof");
    

    start_timer();
    for (i = 0; i != iterations; ++i)
        {
        double sum = 0;
        for (j = 0; j < SIZE; ++j )
            {
            std::string temp( float_strings[j] );
            sum += std::stod (temp);
            }
        check_sum_double(sum);
        }
    record_result( timer(), "std::stod");


#if 0 && __cplusplus >= 201703
// Not yet implemented in LLVM/Clang or GCC
// compiles in latest GCC, but not in many Linux distributions
    start_timer();
    for (i = 0; i != iterations; ++i)
        {
        double sum = 0;
        for (j = 0; j < SIZE; ++j )
            {
            double value;
            (void)std::from_chars( float_strings[j], float_strings[j]+max_number_size, value );
            sum += value;
            }
        check_sum_double(sum);
        }
    record_result( timer(), "std::from_chars double");
#endif


    start_timer();
    for (i = 0; i != iterations; ++i)
        {
        double sum = 0.0;
        for (j = 0; j < SIZE; ++j )
            {
            char *input = float_strings[j];
            double temp = simple_strtod ( input, NULL );
            sum += temp;
            }
        check_sum_double(sum);
        }
    record_result( timer(), "simple_strtod");

    
    summarize("atof", SIZE, iterations, kDontShowGMeans, kDontShowPenalty );
}

/******************************************************************************/

void testFloatSci()
{
    int i, j;

    start_timer();
    for (i = 0; i != iterations; ++i)
        {
        float sum = 0.0;
        for (j = 0; j < SIZE; ++j )
            {
            sum += atof ( float_stringsE[j] );
            }
        check_sum_float(sum);
        }
    record_result( timer(), "atof E");
    
    
    start_timer();
    for (i = 0; i != iterations; ++i)
        {
        float sum = 0.0;
        for (j = 0; j < SIZE; ++j )
            {
            sum += strtof ( float_stringsE[j], NULL );
            }
        check_sum_float(sum);
        }
    record_result( timer(), "strtof E");


    start_timer();
    for (i = 0; i != iterations; ++i)
        {
        double sum = 0.0;
        for (j = 0; j < SIZE; ++j )
            {
            sum += strtod ( float_stringsE[j], NULL );
            }
        check_sum_double(sum);
        }
    record_result( timer(), "strtod E");
    
    
    start_timer();
    for (i = 0; i != iterations; ++i)
        {
        float sum = 0.0;
        for (j = 0; j < SIZE; ++j )
            {
            float result;
            sscanf( float_stringsE[j], "%f", &result );
            sum += result;
            }
        check_sum_float(sum);
        }
    record_result( timer(), "sscanf f float E");
    
    
    start_timer();
    for (i = 0; i != iterations; ++i)
        {
        float sum = 0.0;
        for (j = 0; j < SIZE; ++j )
            {
            float result;
            sscanf( float_stringsE[j], "%g", &result );
            sum += result;
            }
        check_sum_float(sum);
        }
    record_result( timer(), "sscanf g float E");
    
    
    start_timer();
    for (i = 0; i != iterations; ++i)
        {
        double sum = 0.0;
        for (j = 0; j < SIZE; ++j )
            {
            double result;
            sscanf( float_stringsE[j], "%lf", &result );
            sum += result;
            }
        check_sum_double(sum);
        }
    record_result( timer(), "sscanf f double E");
    
    
    start_timer();
    for (i = 0; i != iterations; ++i)
        {
        double sum = 0.0;
        for (j = 0; j < SIZE; ++j )
            {
            double result;
            sscanf( float_stringsE[j], "%lg", &result );
            sum += result;
            }
        check_sum_double(sum);
        }
    record_result( timer(), "sscanf g double E");
    

    start_timer();
    for (i = 0; i != iterations; ++i)
        {
        float sum = 0;
        for (j = 0; j < SIZE; ++j )
            {
            std::string temp( float_stringsE[j] );
            sum += std::stof (temp);
            }
        check_sum_float(sum);
        }
    record_result( timer(), "std::stof E");
    

    start_timer();
    for (i = 0; i != iterations; ++i)
        {
        double sum = 0;
        for (j = 0; j < SIZE; ++j )
            {
            std::string temp( float_stringsE[j] );
            sum += std::stod (temp);
            }
        check_sum_double(sum);
        }
    record_result( timer(), "std::stod E");


#if 0 && __cplusplus >= 201703
// Not yet implemented in LLVM/Clang or GCC
// compiles in latest GCC, but not in many Linux distributions
    start_timer();
    for (i = 0; i != iterations; ++i)
        {
        double sum = 0;
        for (j = 0; j < SIZE; ++j )
            {
            double value;
            (void)std::from_chars( float_stringsE[j], float_stringsE[j]+max_number_size, value, std::chars_format::scientific );
            sum += value;
            }
        check_sum_double(sum);
        }
    record_result( timer(), "std::from_chars E");
#endif


    start_timer();
    for (i = 0; i != iterations; ++i)
        {
        double sum = 0.0;
        for (j = 0; j < SIZE; ++j )
            {
            char *input = float_stringsE[j];
            double temp = simple_strtod ( input, NULL );
            sum += temp;
            }
        check_sum_double(sum);
        }
    record_result( timer(), "simple_strtod E");

    
    summarize("atof E", SIZE, iterations, kDontShowGMeans, kDontShowPenalty );
}

/******************************************************************************/

struct int32StringTest {
    int32_t value;
    const char *string;
};

struct int64StringTest {
    int64_t value;
    const char *string;
};

struct float32StringTest {
    float value;
    const char *string;
};

struct float64StringTest {
    double value;
    const char *string;
};

/******************************************************************************/

void UnitTest()
{
    int32StringTest testValuesInt32[] = {
        { 0, "0" },
        { -0, "-0" },
        { 1, "1" },
        { -1, "-1" },
        { -9, "-9" },
        { 22, "22" },
        { -22, "-22" },
        { -333, "-333" },
        { 7777777, "7777777" },
        { -88888888, "-88888888" },
        { 1111111111, "1111111111" },
        { -1111111111, "-1111111111" },
        { 2147483647, "2147483647" },
        { -2147483647, "-2147483647" }, // MSVC can't handle -2147483648
        
        { 0, "ZZZ" }, // error case
        { 0, "NaN" }, // error case
        { 0, "Because they can't float!" }, // error case
    };
    
    size_t testCountLong = sizeof(testValuesInt32) / sizeof(testValuesInt32[0]);
    
    for (size_t i = 0; i <testCountLong; ++i ) {
        int32_t result0 = atoi( testValuesInt32[i].string );
        if (result0 != testValuesInt32[i].value)
            printf("atoi int32 unit test failed with \"%s\", returned %d\n", testValuesInt32[i].string, result0 );
        
        int32_t result1 = atol( testValuesInt32[i].string );
        if (result1 != testValuesInt32[i].value)
            printf("atol int32 unit test failed with \"%s\", returned %d\n", testValuesInt32[i].string, result1 );
        
        int32_t result2 = strtol( testValuesInt32[i].string, NULL, 0 );
        if (result2 != testValuesInt32[i].value)
            printf("strtol int32 unit test failed with \"%s\", returned %d\n", testValuesInt32[i].string, result2 );
        
        if (testValuesInt32[i].value > 0) {
            int32_t result3 = strtoul( testValuesInt32[i].string, NULL, 0 );
            if (result3 != testValuesInt32[i].value)
                printf("strtoul int32 unit test failed with \"%s\", returned %d\n", testValuesInt32[i].string, result3 );
        }
        
        int32_t result4 = 0;
        sscanf( testValuesInt32[i].string, "%d", &result4 );
        if (result4 != testValuesInt32[i].value)
            printf("sscanf ld int32 unit test failed with \"%s\", returned %d\n", testValuesInt32[i].string, result4 );
        
        int32_t result5 = atoll( testValuesInt32[i].string );
        if (result5 != testValuesInt32[i].value)
            printf("atoll int32 unit test failed with \"%s\", returned %d\n", testValuesInt32[i].string, result5 );
        
        int32_t result6 = strtoll( testValuesInt32[i].string, NULL, 0 );
        if (result6 != testValuesInt32[i].value)
            printf("strtoll int32 unit test failed with \"%s\", returned %d\n", testValuesInt32[i].string, result6 );
        
        if (testValuesInt32[i].value > 0) {
            int32_t result7 = strtoull( testValuesInt32[i].string, NULL, 0 );
            if (result7 != testValuesInt32[i].value)
                printf("strtoull int32 unit test failed with \"%s\", returned %d\n", testValuesInt32[i].string, result7 );
        }
        
        int64_t result8 = 0;
        sscanf( testValuesInt32[i].string, "%" SCNi64, &result8 );
        if (result8 != testValuesInt32[i].value)
            printf("sscanf 64 int32 unit test failed with \"%s\", returned %lld\n", testValuesInt32[i].string, result8 );
        
        int32_t result9 = simple_strtol( testValuesInt32[i].string, NULL, 0 );
        if (result9 != testValuesInt32[i].value)
            printf("simple_strtol int32 unit test failed with \"%s\", returned %d\n", testValuesInt32[i].string, result9 );
        
        
        std::string temp( testValuesInt32[i].string );
        try {
            int32_t result7 = std::stoi (temp);
            if (result7 != testValuesInt32[i].value)
            printf("std::stoi int32 unit test failed with \"%s\", returned %d\n", testValuesInt32[i].string, result7 );
        }
        catch (...) {
                // stoi throws when it encounters non-numeric data
        }
        
        try {
            int32_t result8 = std::stol (temp);
            if (result8 != testValuesInt32[i].value)
            printf("std::stol int32 unit test failed with \"%s\", returned %d\n", testValuesInt32[i].string, result8 );
        }
        catch (...) {
                // stol throws when it encounters non-numeric data
        }
        
        try {
            int32_t result8 = std::stoul (temp);
            if (result8 != testValuesInt32[i].value)
            printf("std::stoul int32 unit test failed with \"%s\", returned %u\n", testValuesInt32[i].string, result8 );
        }
        catch (...) {
                // stoul throws when it encounters non-numeric data
        }
        
        try {
            int32_t result9 = std::stoll (temp);
            if (result9 != testValuesInt32[i].value)
            printf("std::stoll int32 unit test failed with \"%s\", returned %d\n", testValuesInt32[i].string, result9 );
        }
        catch (...) {
                // stoll throws when it encounters non-numeric data
        }
        
        try {
            int32_t result9 = std::stoull (temp);
            if (result9 != testValuesInt32[i].value)
            printf("std::stoull int32 unit test failed with \"%s\", returned %d\n", testValuesInt32[i].string, result9 );
        }
        catch (...) {
                // stoull throws when it encounters non-numeric data
        }

#if __cplusplus >= 201703
        int32_t resultA;
        auto lame_result = std::from_chars( testValuesInt32[i].string, testValuesInt32[i].string+max_number_size, resultA );
        if ((lame_result.ec == std::errc()) && resultA != testValuesInt32[i].value)
            printf("std::from_chars int32 unit test failed with \"%s\", returned %d\n", testValuesInt32[i].string, resultA );
#endif

    }
    
    
    int64StringTest testValuesInt64[] = {
        { 0, "0" },
        { -0, "-0" },
        { 1, "1" },
        { -1, "-1" },
        { -9, "-9" },
        { 22, "22" },
        { -22, "-22" },
        { -333, "-333" },
        { 7777777, "7777777" },
        { -88888888, "-88888888" },
        { 1111111111, "1111111111" },
        { -1111111111, "-1111111111" },
        { 2147483647, "2147483647" },
        { -2147483647, "-2147483647" }, // MSVC can't correctly handle -2147483648
        { 444444444444, "444444444444" },
        { 555555555555555, "555555555555555" },
        { -9223372036854775807, "-9223372036854775807" },   // LLVM can't handle -9223372036854775808
        { 9223372036854775807, "9223372036854775807" },
        
        { 0, "ZZZ" }, // error case
        { 0, "NaN" }, // error case
        { 0, "Half of all integers!" }, // error case
    };
    
    size_t testCount64Long = sizeof(testValuesInt64) / sizeof(testValuesInt64[0]);
    
    for (size_t i = 0; i <testCount64Long; ++i ) {
        
        if (sizeof(long) == 8) {
            long result1 = atol( testValuesInt64[i].string );
            if (result1 != testValuesInt64[i].value)
                printf("atol int64 unit test failed with \"%s\", returned %ld\n", testValuesInt64[i].string, result1 );
            
            long result2 = strtol( testValuesInt64[i].string, NULL, 0 );
            if (result2 != testValuesInt64[i].value)
                printf("strtol int64 unit test failed with \"%s\", returned %ld\n", testValuesInt64[i].string, result2 );
            
            if (testValuesInt64[i].value > 0) {
                long result3 = strtoul( testValuesInt64[i].string, NULL, 0 );
                if (result3 != testValuesInt64[i].value)
                    printf("strtoul int64 unit test failed with \"%s\", returned %lu\n", testValuesInt64[i].string, result3 );
            }
            
            long result4 = 0;
            sscanf( testValuesInt64[i].string, "%ld", &result4 );
            if (result4 != testValuesInt64[i].value)
                printf("sscanf ld int64 unit test failed with \"%s\", returned %ld\n", testValuesInt64[i].string, result4 );
        }
        
        long long result5 = atoll( testValuesInt64[i].string );
        if (result5 != testValuesInt64[i].value)
            printf("atoll int64 unit test failed with \"%s\", returned %lld\n", testValuesInt64[i].string, result5 );
        
        long long result6 = strtoll( testValuesInt64[i].string, NULL, 0 );
        if (result6 != testValuesInt64[i].value)
            printf("strtoll int64 unit test failed with \"%s\", returned %lld\n", testValuesInt64[i].string, result6 );
        
        if (testValuesInt64[i].value > 0) {
            long long result7 = strtoull( testValuesInt64[i].string, NULL, 0 );
            if (result7 != testValuesInt64[i].value)
                printf("strtoull int64 unit test failed with \"%s\", returned %llu\n", testValuesInt64[i].string, result7 );
        }
        
        long long result8 = 0;
        sscanf( testValuesInt64[i].string, "%" SCNi64, &result8 );
        if (result8 != testValuesInt64[i].value)
            printf("sscanf 64 int64 unit test failed with \"%s\", returned %lld\n", testValuesInt64[i].string, result8 );
        
        int64_t result9 = simple_strtol( testValuesInt64[i].string, NULL, 0 );
        if (result9 != testValuesInt64[i].value)
            printf("simple_strtol int64 unit test failed with \"%s\", returned %lld\n", testValuesInt64[i].string, result9 );
        
        
        std::string temp( testValuesInt64[i].string );
        
        if (sizeof(long) == 8) {
            try {
                long result8 = std::stol (temp);
                if (result8 != testValuesInt64[i].value)
                printf("std::stol int64 unit test failed with \"%s\", returned %ld\n", testValuesInt64[i].string, result8 );
            }
            catch (...) {
                    // stol throws when it encounters non-numeric data
            }
        
            try {
                unsigned long result8 = std::stoul (temp);
                if (result8 != testValuesInt64[i].value)
                printf("std::stoul int64 unit test failed with \"%s\", returned %lu\n", testValuesInt64[i].string, result8 );
            }
            catch (...) {
                    // stoul throws when it encounters non-numeric data
            }
        }
        
        try {
            long long result9 = std::stoll (temp);
            if (result9 != testValuesInt64[i].value)
            printf("std::stoll int64 unit test failed with \"%s\", returned %lld\n", testValuesInt64[i].string, result9 );
        }
        catch (...) {
                // stoll throws when it encounters non-numeric data
        }
        
        try {
            unsigned long long result9 = std::stoull (temp);
            if (result9 != testValuesInt64[i].value)
            printf("std::stoull int64 unit test failed with \"%s\", returned %lld\n", testValuesInt64[i].string, result9 );
        }
        catch (...) {
                // stoull throws when it encounters non-numeric data
        }

#if __cplusplus >= 201703
        int64_t resultA;
        auto lame_result = std::from_chars( testValuesInt64[i].string, testValuesInt64[i].string+max_number_size, resultA );
        if ((lame_result.ec == std::errc()) && resultA != testValuesInt64[i].value)
            printf("std::from_chars int64 unit test failed with \"%s\", returned %lld\n", testValuesInt64[i].string, resultA );
#endif

    }



    // HEX
    int32StringTest testValuesHex32[] = {
        { 0, "0x0" },
        { 0, "0x00000000" },
        { 1, "0x1" },
        { 0xff, "0xff" },
        { 0xff, "0xFF" },
        { 0xff, "0xFf" },
        { 0xffff, "0xFfFf" },
        { 0xfffff, "0xfffff" },
        { 0xaaaaaa, "0xaaaaaa" },
        { 0xbbbbbbb, "0xbbbbbbb" },
        { 0x7fffffff, "0x7fffffff" },
        
        { 0, "ZZZ" }, // error case
        { 0, "NaN" }, // error case
        { 0, "string walks into a bar" }, // error case
    };
    
    size_t testCountHex32 = sizeof(testValuesHex32) / sizeof(testValuesHex32[0]);
    
    for (size_t i = 0; i <testCountHex32; ++i ) {
        unsigned long result0 = strtol( testValuesHex32[i].string, NULL, 16 );
        if (result0 != testValuesHex32[i].value)
            printf("strtol hex32 unit test failed with \"%s\", returned %lx\n", testValuesHex32[i].string, result0 );
        
        unsigned long result1 = strtoul( testValuesHex32[i].string, NULL, 16 );
        if (result1 != testValuesHex32[i].value)
            printf("strtoul hex32 unit test failed with \"%s\", returned %lx\n", testValuesHex32[i].string, result1 );
        
        unsigned long result2 = 0;
        sscanf( testValuesHex32[i].string, "%lx", &result2 );
        if (result2 != testValuesHex32[i].value)
            printf("sscanf hex32 unit test failed with \"%s\", returned %lx\n", testValuesHex32[i].string, result2 );
        
        unsigned long long result3 = strtoll( testValuesHex32[i].string, NULL, 16 );
        if (result3 != testValuesHex32[i].value)
            printf("strtoll hex32 unit test failed with \"%s\", returned %llx\n", testValuesHex32[i].string, result3 );
        
        unsigned long long result4 = strtoull( testValuesHex32[i].string, NULL, 16 );
        if (result4 != testValuesHex32[i].value)
            printf("strtoull hex32 unit test failed with \"%s\", returned %llx\n", testValuesHex32[i].string, result4 );
        
        unsigned long long result5 = 0;
        sscanf( testValuesHex32[i].string, "%" SCNx64, &result5 );
        if (result5 != testValuesHex32[i].value)
            printf("sscanf 64 hex32 unit test failed with \"%s\", returned %llx\n", testValuesHex32[i].string, result5 );
        
        uint32_t result6 = simple_strtol( testValuesHex32[i].string, NULL, 16 );
        if (result6 != testValuesHex32[i].value)
            printf("simple_strtol hex32 unit test failed with \"%s\", returned %x\n", testValuesHex32[i].string, result6 );
        
        
        std::string temp( testValuesHex32[i].string );
        try {
            unsigned long result7 = std::stoi (temp, NULL, 16);
            if (result7 != testValuesHex32[i].value)
            printf("std::stoi hex32 unit test failed with \"%s\", returned %lx\n", testValuesHex32[i].string, result7 );
        }
        catch (...) {
                // stoi throws when it encounters non-numeric data
        }
        
        try {
            unsigned long result8 = std::stol (temp, NULL, 16);
            if (result8 != testValuesHex32[i].value)
            printf("std::stol hex32 unit test failed with \"%s\", returned %lx\n", testValuesHex32[i].string, result8 );
        }
        catch (...) {
                // stol throws when it encounters non-numeric data
        }
        
        try {
            unsigned long result8 = std::stoul (temp, NULL, 16);
            if (result8 != testValuesHex32[i].value)
            printf("std::stoul hex32 unit test failed with \"%s\", returned %lx\n", testValuesHex32[i].string, result8 );
        }
        catch (...) {
                // stoul throws when it encounters non-numeric data
        }
        
        try {
            unsigned long long result9 = std::stoll (temp, NULL, 16);
            if (result9 != testValuesHex32[i].value)
            printf("std::stoll hex32 unit test failed with \"%s\", returned %llx\n", testValuesHex32[i].string, result9 );
        }
        catch (...) {
                // stoll throws when it encounters non-numeric data
        }
        
        try {
            unsigned long long result9 = std::stoull (temp, NULL, 16);
            if (result9 != testValuesHex32[i].value)
            printf("std::stoull hex32 unit test failed with \"%s\", returned %llx\n", testValuesHex32[i].string, result9 );
        }
        catch (...) {
                // stoull throws when it encounters non-numeric data
        }

#if __cplusplus >= 201703
// cannot handle 0x prefix
        long resultA;
        auto lame_result = std::from_chars( testValuesHex32[i].string+2, testValuesHex32[i].string+max_number_size, resultA, 16 );
        if ((lame_result.ec == std::errc()) && resultA != testValuesHex32[i].value)
            printf("std::from_chars hex32 unit test failed with \"%s\", returned %ld\n", testValuesHex32[i].string, resultA );
#endif

    }
    
    
    // HEX
    int64StringTest testValuesHex64[] = {
        { 0, "0x0" },
        { 0, "0x00000000" },
        { 1, "0x1" },
        { 0xff, "0xff" },
        { 0xff, "0xFF" },
        { 0xff, "0xFf" },
        { 0xffff, "0xFfFf" },
        { 0xfffff, "0xfffff" },
        { 0xaaaaaa, "0xaaaaaa" },
        { 0xbbbbbbb, "0xbbbbbbb" },
        { 0xcccccccc, "0xcccccccc" },
        { 0xffffffff, "0xffffffff" },
        { 0x7ccccccccccccccc, "0x7ccccccccccccccc" },
        { 0x7fffffffffffffff, "0x7fffffffffffffff" },
        
        { 0, "ZZZ" }, // error case
        { 0, "NaN" }, // error case
        { 0, "...and those who weren't expecting a double-subverted binary joke." }, // error case
    };
    
    size_t testCountHex64 = sizeof(testValuesHex64) / sizeof(testValuesHex64[0]);
    
    for (size_t i = 0; i <testCountHex64; ++i ) {
        
        if (sizeof(long) == 8) {
            unsigned long result0 = strtol( testValuesHex64[i].string, NULL, 16 );
            if (result0 != testValuesHex64[i].value)
                printf("strtol hex64 unit test failed with \"%s\", returned %lx\n", testValuesHex64[i].string, result0 );
            
            unsigned long result1 = strtoul( testValuesHex64[i].string, NULL, 16 );
            if (result1 != testValuesHex64[i].value)
                printf("strtoul hex64 unit test failed with \"%s\", returned %lx\n", testValuesHex64[i].string, result1 );
            
            unsigned long result2 = 0;
            sscanf( testValuesHex64[i].string, "%lx", &result2 );
            if (result2 != testValuesHex64[i].value)
                printf("sscanf hex64 unit test failed with \"%s\", returned %lx\n", testValuesHex64[i].string, result2 );
        }
        
        unsigned long long result3 = strtoll( testValuesHex64[i].string, NULL, 16 );
        if (result3 != testValuesHex64[i].value)
            printf("strtoll hex64 unit test failed with \"%s\", returned %llx\n", testValuesHex64[i].string, result3 );
        
        unsigned long long result4 = strtoull( testValuesHex64[i].string, NULL, 16 );
        if (result4 != testValuesHex64[i].value)
            printf("strtoull hex64 unit test failed with \"%s\", returned %llx\n", testValuesHex64[i].string, result4 );
        
        unsigned long long result5 = 0;
        sscanf( testValuesHex64[i].string, "%" SCNx64, &result5 );
        if (result5 != testValuesHex64[i].value)
            printf("sscanf 64 hex64 unit test failed with \"%s\", returned %llx\n", testValuesHex64[i].string, result5 );
        
        int64_t result6 = simple_strtol( testValuesHex64[i].string, NULL, 16 );
        if (result6 != testValuesHex64[i].value)
            printf("simple_strtol hex64 unit test failed with \"%s\", returned %llx\n", testValuesHex64[i].string, result6 );
        
        
        std::string temp( testValuesHex64[i].string );

        if (sizeof(long) == 8) {
            try {
                unsigned long result8 = std::stol (temp, NULL, 16);
                if (result8 != testValuesHex64[i].value)
                printf("std::stol hex64 unit test failed with \"%s\", returned %lx\n", testValuesHex64[i].string, result8 );
            }
            catch (...) {
                    // stol throws when it encounters non-numeric data
            }
            
            try {
                unsigned long result8 = std::stoul (temp, NULL, 16);
                if (result8 != testValuesHex64[i].value)
                printf("std::stoul hex64 unit test failed with \"%s\", returned %lx\n", testValuesHex64[i].string, result8 );
            }
            catch (...) {
                    // stoul throws when it encounters non-numeric data
            }
        }
        
        try {
            unsigned long long result9 = std::stoll (temp, NULL, 16);
            if (result9 != testValuesHex64[i].value)
            printf("std::stoll hex64 unit test failed with \"%s\", returned %llx\n", testValuesHex64[i].string, result9 );
        }
        catch (...) {
                // stoll throws when it encounters non-numeric data
        }
        
        try {
            unsigned long long result9 = std::stoull (temp, NULL, 16);
            if (result9 != testValuesHex64[i].value)
            printf("std::stoull hex64 unit test failed with \"%s\", returned %llx\n", testValuesHex64[i].string, result9 );
        }
        catch (...) {
                // stoull throws when it encounters non-numeric data
        }

#if __cplusplus >= 201703
// cannot handle 0x prefix
        int64_t resultA;
        auto lame_result = std::from_chars( testValuesHex64[i].string+2, testValuesHex64[i].string+max_number_size, resultA, 16 );
        if ((lame_result.ec == std::errc()) && resultA != testValuesHex64[i].value)
            printf("std::from_chars hex64 unit test failed with \"%s\", returned %lld\n", testValuesHex64[i].string, resultA );
#endif

    }
    
    
    
    
    // Float & Exponent
    float32StringTest testValuesFloat32[] = {
        { 0.0, "0" },
        { 1.0, "1" },
        { -1.0, "-1" },
        { -1.0, "-1.0" },
        { -1.0, "-1.0000000000000000000000000000000000" },
        { 3.14, "3.140000" },
        { 999999.2, "999999.200000" },
        { -999999.2, "-999999.200000" },
        { 4.222222, "4.222222" },
        { -4.222222, "-4.222222" },
        { 4.333333333333333, "4.333333333333333" },
        
        { 1e0, "1e0" },         // aka 1.0
        { 1e000, "1e000" },     // aka 1.0
        { 1.0e-0, "1.0e-0" },   // aka 1.0
        { -2.0e-4, "-2.0e-4" },
        { -2.e-4, "-2.e-4" },
        { 2e004, "2e004" },
        { 2e00004, "2e00004" },     // not exactly a normal exponent, but should parse
        { -2e-4, "-2e-4" },
        
        { INFINITY, "INFINITY" }, // expected behavior
        { INFINITY, "infinity" }, // expected behavior
        { INFINITY, "Infinity" }, // expected behavior
        { NAN, "NAN" }, // expected behavior
        { NAN, "NaN" }, // expected behavior
        { NAN, "nan" }, // expected behavior
        
        //{ 0, "5e9999" }, // error case - implementations don't all match documentation
        { 0, "ZZZ" }, // error case
        { 0, "QA walks into a foobaz" }, // error case
    };
    
    size_t testCountFloat32 = sizeof(testValuesFloat32) / sizeof(testValuesFloat32[0]);
    const double maxFloatTestVal = 1e38;
    const float epsilon32 = 1e-7;
    
    for (size_t i = 0; i <testCountFloat32; ++i ) {
        double result0 = atof( testValuesFloat32[i].string );
        if ((result0 == result0) && fabs( (result0 - testValuesFloat32[i].value) / result0 ) > epsilon32 )    // don't give errors on NaN, because they are NaN
            printf("atoi float32 unit test failed with \"%s\", returned %lf\n", testValuesFloat32[i].string, result0 );
        
        float result1 = strtof( testValuesFloat32[i].string, NULL );
        if ((result1 == result1) && fabs( (result1 - testValuesFloat32[i].value) / result1 ) > epsilon32 )
            printf("strtof float32 unit test failed with \"%s\", returned %f\n", testValuesFloat32[i].string, result1 );

        double result2 = strtod( testValuesFloat32[i].string, NULL );
        if ((result2 == result2) && fabs( (result2 - testValuesFloat32[i].value) / result2 ) > epsilon32 )    // don't give errors on NaN, because they are NaN
            printf("strtod float32 unit test failed with \"%s\", returned %lf\n", testValuesFloat32[i].string, result2 );
    
        if (fabs(testValuesFloat32[i].value) < maxFloatTestVal) {
            float result3 = 0;
            sscanf( testValuesFloat32[i].string, "%f", &result3 );
            if ((result3 == result3) && fabs( (result3 - testValuesFloat32[i].value) / result3 ) > epsilon32 )
                printf("sscanf f float32 unit test failed with \"%s\", returned %f\n", testValuesFloat32[i].string, result3 );
        }
        
        if (fabs(testValuesFloat32[i].value) < maxFloatTestVal) {
            float result4 = 0;
            sscanf( testValuesFloat32[i].string, "%g", &result4 );
            if ((result4 == result4) && fabs( (result4 - testValuesFloat32[i].value) / result4 ) > epsilon32 )
                printf("sscanf g float32 unit test failed with \"%s\", returned %f\n", testValuesFloat32[i].string, result4 );
        }
        
        double result5 = 0;
        sscanf( testValuesFloat32[i].string, "%lf", &result5 );
        if ((result5 == result5) && fabs( (result5 - testValuesFloat32[i].value) / result5 ) > epsilon32 )
            printf("sscanf lf float32 unit test failed with \"%s\", returned %lf\n", testValuesFloat32[i].string, result5 );
    
        double result6 = 0;
        sscanf( testValuesFloat32[i].string, "%lg", &result6 );
        if ((result6 == result6) && fabs( (result6 - testValuesFloat32[i].value) / result6 ) > epsilon32 )
            printf("sscanf lg float32 unit test failed with \"%s\", returned %lf\n", testValuesFloat32[i].string, result6 );

        double result7 = simple_strtod( testValuesFloat32[i].string, NULL );
        if ((result7 == result7) && fabs( (result7 - testValuesFloat32[i].value) / result7 ) > epsilon32 )    // don't give errors on NaN, because they are NaN
            printf("simple_strtod float32 unit test failed with \"%s\", returned %lf\n", testValuesFloat32[i].string, result7 );
        
        std::string temp( testValuesFloat32[i].string );
        try {
            float result8 = std::stof (temp);
            if ((result8 == result8) && fabs( (result8 - testValuesFloat32[i].value) / result8 ) > epsilon32 )    // don't give errors on NaN, because they are NaN
                printf("std::stof float32 unit test failed with \"%s\", returned %lf\n", testValuesFloat32[i].string, result8 );
            }
        catch (...) {
            // stof throws when it encounters non-numeric data
        }
        
        try {
            double result9 = std::stod (temp);
            if ((result9 == result9) && fabs( (result9 - testValuesFloat32[i].value) / result9 ) > epsilon32 )    // don't give errors on NaN, because they are NaN
                printf("std::stod float32 unit test failed with \"%s\", returned %lf\n", testValuesFloat32[i].string, result9 );
        }
        catch (...) {
                // stod throws when it encounters non-numeric data
        }

#if 0 && __cplusplus >= 201703
// Not yet implemented in LLVM/Clang or GCC
// compiles in latest GCC, but not in many Linux distributions
        double resultA;
        auto lame_result = std::from_chars( testValuesFloat32[i].string, testValuesFloat32[i].string+max_number_size, resultA );
        if ((lame_result.ec == std::errc()) && fabs( (resultA - testValuesFloat32[i].value) / resultA ) > epsilon32 )
            printf("std::from_chars float float32 unit test failed with \"%s\", returned %lf\n", testValuesFloat32[i].string, resultA );
#endif

#if 0 && __cplusplus >= 201703
// Not yet implemented in LLVM/Clang or GCC
// compiles in latest GCC, but not in many Linux distributions
        double resultB;
        auto lame_result2 = std::from_chars( testValuesFloat32[i].string, testValuesFloat32[i].string+max_number_size, resultB, std::chars_format::scientific  );
        if ((lame_result2.ec == std::errc()) && fabs( (resultB - testValuesFloat32[i].value) / resultB ) > epsilon32 )
            printf("std::from_chars float float32 unit test failed with \"%s\", returned %lf\n", testValuesFloat32[i].string, resultB );
#endif
    }
    
    
    // Float & Exponent
    float64StringTest testValuesFloat64[] = {
        { 0.0, "0" },
        { 1.0, "1" },
        { -1.0, "-1" },
        { -1.0, "-1.0" },
        { -1.0, "-1.0000000000000000000000000000000000" },
        { 3.14, "3.14" },
        { 999999.2, "999999.2" },
        { -999999.2, "-999999.2" },
        { 4.222222, "4.222222" },
        { -4.222222, "-4.222222" },
        { 4.333333333333333, "4.333333333333333" },
        
        { 1e0, "1e0" },         // aka 1.0
        { 1e000, "1e000" },     // aka 1.0
        { 1.0e-0, "1.0e-0" },   // aka 1.0
        { -2.0e-4, "-2.0e-4" },
        { -2.e-4, "-2.e-4" },
        { 2e004, "2e004" },
        { 2e00004, "2e00004" },     // not exactly a normal exponent, but should parse
        { -2e-4, "-2e-4" },
        { 2.50e42, "2.50e42" },
        { 7.00e55, "7.00e55" },
        { 2.2111111e88, "2.2111111e88" },
        { 2.4111111e300, "2.4111111e300" },
        
        { INFINITY, "INFINITY" }, // expected behavior
        { INFINITY, "infinity" }, // expected behavior
        { INFINITY, "Infinity" }, // expected behavior
        { NAN, "NAN" }, // expected behavior
        { NAN, "NaN" }, // expected behavior
        { NAN, "nan" }, // expected behavior
        
        //{ 0, "5e9999" }, // error case - but implementations don't all match documentation
        { 0, "ZZZ" }, // error case
        { 0, "https://www.smbc-comics.com/index.php?db=comics&id=2999" }, // error case
    };
    
    size_t testCountFloat64 = sizeof(testValuesFloat64) / sizeof(testValuesFloat64[0]);
    const double epsilon64 = 1e-20;
    
    for (size_t i = 0; i <testCountFloat64; ++i ) {
        double result0 = atof( testValuesFloat64[i].string );
        if ((result0 == result0) && fabs( (result0 - testValuesFloat64[i].value) / result0 ) > epsilon64 )    // don't give errors on NaN, because they are NaN
            printf("atoi float64 unit test failed with \"%s\", returned %lf\n", testValuesFloat64[i].string, result0 );

        double result2 = strtod( testValuesFloat64[i].string, NULL );
        if ((result2 == result2) && fabs( (result2 - testValuesFloat64[i].value) / result2 ) > epsilon64 )    // don't give errors on NaN, because they are NaN
            printf("strtod float64 unit test failed with \"%s\", returned %lf\n", testValuesFloat64[i].string, result2 );

        double result5 = 0;
        sscanf( testValuesFloat64[i].string, "%lf", &result5 );
        if ((result5 == result5) && fabs( (result5 - testValuesFloat64[i].value) / result5 ) > epsilon64 )
            printf("sscanf lf float64 unit test failed with \"%s\", returned %lf\n", testValuesFloat64[i].string, result5 );
    
        double result6 = 0;
        sscanf( testValuesFloat64[i].string, "%lg", &result6 );
        if ((result6 == result6) && fabs( (result6 - testValuesFloat64[i].value) / result6 ) > epsilon64 )
            printf("sscanf lg float64 unit test failed with \"%s\", returned %lf\n", testValuesFloat64[i].string, result6 );

        double result7 = simple_strtod( testValuesFloat64[i].string, NULL );
        if ((result7 == result7) && fabs( (result7 - testValuesFloat64[i].value) / result7 ) > epsilon64 )    // don't give errors on NaN, because they are NaN
            printf("simple_strtod float64 unit test failed with \"%s\", returned %lf\n", testValuesFloat64[i].string, result7 );
        
        std::string temp( testValuesFloat64[i].string );
        try {
            double result9 = std::stod (temp);
            if ((result9 == result9) && fabs( (result9 - testValuesFloat64[i].value) / result9 ) > epsilon64 )    // don't give errors on NaN, because they are NaN
                printf("std::stod float64 unit test failed with \"%s\", returned %lf\n", testValuesFloat64[i].string, result9 );
        }
        catch (...) {
                // stod throws when it encounters non-numeric data
        }

#if 0 && __cplusplus >= 201703
// not yet implemented in LLVM or GCC
        double resultA;
        auto lame_result = std::from_chars( testValuesFloat64[i].string, testValuesFloat64[i].string+max_number_size, resultA );
        if ((lame_result.ec == std::errc()) && fabs( (resultA - testValuesFloat64[i].value) / resultA ) > epsilon64 )
            printf("std::from_chars float64 unit test failed with \"%s\", returned %lf\n", testValuesFloat64[i].string, resultA );
#endif

#if 0 && __cplusplus >= 201703
// not yet implemented in LLVM or GCC
        double resultB;
        auto lame_result2 = std::from_chars( testValuesFloat64[i].string, testValuesFloat64[i].string+max_number_size, resultB, std::chars_format::scientific  );
        if ((lame_result2.ec == std::errc()) && fabs( (resultB - testValuesFloat64[i].value) / resultB ) > epsilon64 )
            printf("std::from_chars float646 unit test failed with \"%s\", returned %lf\n", testValuesFloat64[i].string, resultB );
#endif
    }
    
    
    
    // binary
    int32StringTest testValuesBinary32[] = {
        { 0, "0" },
        { 1, "1" },
        { 1, "01" },
        { 1, "00000001" },
        { 2, "10" },
        { 2, "0010" },
        { 3, "11" },
        { 7, "111" },
        { 15, "1111" },
        { 16, "10000" },
        { 31, "11111" },
        { 63, "111111" },
        { 127, "1111111" },
        { 255, "11111111" },
        { 256, "100000000" },
        { 511, "111111111" },
        { 1023, "1111111111" },
        { 2047, "11111111111" },
        { 4095, "111111111111" },
        { 8191, "1111111111111" },
        { 16383, "11111111111111" },
        { 32767, "111111111111111" },
        { 65535, "1111111111111111" },
        { 65536, "10000000000000000" },
        { 131071, "11111111111111111" },
        { 262143, "111111111111111111" },
        { 524287, "1111111111111111111" },
        { 1048575, "11111111111111111111" },
        { 2147483647, "01111111111111111111111111111111" },
        
        { 0, "2" }, // error case
        { 0, "3" }, // error case
        { 0, "a" }, // error case
        { 0, "f" }, // error case
        { 0, "z" }, // error case
        { 0, "ZZZ" }, // error case
        { 0, "English and Binary" }, // error case
    };
    
    size_t testCountBinary32 = sizeof(testValuesBinary32) / sizeof(testValuesBinary32[0]);
    
    for (size_t i = 0; i <testCountBinary32; ++i ) {
        
        long result2 = strtol( testValuesBinary32[i].string, NULL, 2 );
        if (result2 != testValuesBinary32[i].value)
            printf("strtol binary32 unit test failed with \"%s\", returned %ld\n", testValuesBinary32[i].string, result2 );
    
        long result3 = strtoul( testValuesBinary32[i].string, NULL, 2 );
        if (result3 != testValuesBinary32[i].value)
            printf("strtoul binary32 unit test failed with \"%s\", returned %lu\n", testValuesBinary32[i].string, result3 );
        
        long long result6 = strtoll( testValuesBinary32[i].string, NULL, 2 );
        if (result6 != testValuesBinary32[i].value)
            printf("strtoll binary32 unit test failed with \"%s\", returned %lld\n", testValuesBinary32[i].string, result6 );
        
        long long result7 = strtoull( testValuesBinary32[i].string, NULL, 2 );
        if (result7 != testValuesBinary32[i].value)
            printf("strtoull binary32 unit test failed with \"%s\", returned %llu\n", testValuesBinary32[i].string, result7 );

        int32_t result9 = simple_strtol( testValuesBinary32[i].string, NULL, 2 );
        if (result9 != testValuesBinary32[i].value)
            printf("simple_strtol binary32 unit test failed with \"%s\", returned %d\n", testValuesBinary32[i].string, result9 );
        
        std::string temp( testValuesBinary32[i].string );
        try {
            unsigned long result7 = std::stoi (temp, NULL, 2);
            if (result7 != testValuesBinary32[i].value)
            printf("std::stoi binary32 unit test failed with \"%s\", returned %ld\n", testValuesBinary32[i].string, result7 );
        }
        catch (...) {
                // stoi throws when it encounters non-numeric data
        }
        
        try {
            long result8 = std::stol (temp, NULL, 2);
            if (result8 != testValuesBinary32[i].value)
            printf("std::stol binary32 unit test failed with \"%s\", returned %ld\n", testValuesBinary32[i].string, result8 );
        }
        catch (...) {
                // stol throws when it encounters non-numeric data
        }
        
        try {
            unsigned long result8 = std::stoul (temp, NULL, 2);
            if (result8 != testValuesBinary32[i].value)
            printf("std::stoul binary32 unit test failed with \"%s\", returned %lu\n", testValuesBinary32[i].string, result8 );
        }
        catch (...) {
                // stoul throws when it encounters non-numeric data
        }
        
        try {
            long long result9 = std::stoll (temp, NULL, 2);
            if (result9 != testValuesBinary32[i].value)
            printf("std::stoll binary32 unit test failed with \"%s\", returned %lld\n", testValuesBinary32[i].string, result9 );
        }
        catch (...) {
                // stoll throws when it encounters non-numeric data
        }
        
        try {
            unsigned long long result9 = std::stoull (temp, NULL, 2);
            if (result9 != testValuesBinary32[i].value)
            printf("std::stoull binary32 unit test failed with \"%s\", returned %lld\n", testValuesBinary32[i].string, result9 );
        }
        catch (...) {
                // stoull throws when it encounters non-numeric data
        }

#if __cplusplus >= 201703
        long resultA;
        auto lame_result = std::from_chars( testValuesBinary32[i].string, testValuesBinary32[i].string+max_number_size, resultA, 2 );
        if ((lame_result.ec == std::errc()) && resultA != testValuesBinary32[i].value)
            printf("std::from_chars binary32 unit test failed with \"%s\", returned %ld\n", testValuesBinary32[i].string, resultA );
#endif
    
    }
    
    
    
    // binary
    int64StringTest testValuesBinary64[] = {
        { 0, "0" },
        { 1, "1" },
        { 1, "01" },
        { 1, "00000001" },
        { 2, "10" },
        { 2, "0010" },
        { 3, "11" },
        { 7, "111" },
        { 15, "1111" },
        { 16, "10000" },
        { 31, "11111" },
        { 63, "111111" },
        { 127, "1111111" },
        { 255, "11111111" },
        { 256, "100000000" },
        { 511, "111111111" },
        { 1023, "1111111111" },
        { 2047, "11111111111" },
        { 4095, "111111111111" },
        { 8191, "1111111111111" },
        { 16383, "11111111111111" },
        { 32767, "111111111111111" },
        { 65535, "1111111111111111" },
        { 65536, "10000000000000000" },
        { 131071, "11111111111111111" },
        { 262143, "111111111111111111" },
        { 524287, "1111111111111111111" },
        { 1048575, "11111111111111111111" },
        { 1, "00000000000000000000000000000001" },
        { 2147483647, "01111111111111111111111111111111" },
        { 2147483648, "10000000000000000000000000000000" },
        { 4294967295, "11111111111111111111111111111111" },
        { 1099511627775, "1111111111111111111111111111111111111111" },
        { 4611686018427387903, "0011111111111111111111111111111111111111111111111111111111111111" },
        { 6148914691236517205, "0101010101010101010101010101010101010101010101010101010101010101" },
        { 9223372036854775807, "0111111111111111111111111111111111111111111111111111111111111111" },
        
        { 0, "2" }, // error case
        { 0, "3" }, // error case
        { 0, "a" }, // error case
        { 0, "f" }, // error case
        { 0, "z" }, // error case
        { 0, "ZZZ" }, // error case
        { 0, "and off-by-one errors." }, // error case
    };
    
    size_t testCountBinary64 = sizeof(testValuesBinary64) / sizeof(testValuesBinary64[0]);
    
    for (size_t i = 0; i <testCountBinary64; ++i ) {
        
        if (sizeof(long) == 8) {
            long result2 = strtol( testValuesBinary64[i].string, NULL, 2 );
            if (result2 != testValuesBinary64[i].value)
                printf("strtol binary64 unit test failed with \"%s\", returned %ld\n", testValuesBinary64[i].string, result2 );
        
            long result3 = strtoul( testValuesBinary64[i].string, NULL, 2 );
            if (result3 != testValuesBinary64[i].value)
                printf("strtoul binary64 unit test failed with \"%s\", returned %lu\n", testValuesBinary64[i].string, result3 );
        }
        
        long long result6 = strtoll( testValuesBinary64[i].string, NULL, 2 );
        if (result6 != testValuesBinary64[i].value)
            printf("strtoll binary64 unit test failed with \"%s\", returned %lld\n", testValuesBinary64[i].string, result6 );
        
        long long result7 = strtoull( testValuesBinary64[i].string, NULL, 2 );
        if (result7 != testValuesBinary64[i].value)
            printf("strtoull binary64 unit test failed with \"%s\", returned %llu\n", testValuesBinary64[i].string, result7 );

        int64_t result9 = simple_strtol( testValuesBinary64[i].string, NULL, 2 );
        if (result9 != testValuesBinary64[i].value)
            printf("simple_strtol binary64 unit test failed with \"%s\", returned %lld\n", testValuesBinary64[i].string, result9 );
        
        std::string temp( testValuesBinary64[i].string );
    
        if (sizeof(long) == 8) {
            try {
                long result8 = std::stol (temp, NULL, 2);
                if (result8 != testValuesBinary64[i].value)
                printf("std::stol binary64 unit test failed with \"%s\", returned %ld\n", testValuesBinary64[i].string, result8 );
            }
            catch (...) {
                    // stol throws when it encounters non-numeric data
            }
            
            try {
                unsigned long result8 = std::stoul (temp, NULL, 2);
                if (result8 != testValuesBinary64[i].value)
                printf("std::stoul binary64 unit test failed with \"%s\", returned %lu\n", testValuesBinary64[i].string, result8 );
            }
            catch (...) {
                    // stoul throws when it encounters non-numeric data
            }
        }
        
        try {
            long long result9 = std::stoll (temp, NULL, 2);
            if (result9 != testValuesBinary64[i].value)
            printf("std::stoll binary64 unit test failed with \"%s\", returned %lld\n", testValuesBinary64[i].string, result9 );
        }
        catch (...) {
                // stoll throws when it encounters non-numeric data
        }
        
        try {
            unsigned long long result9 = std::stoull (temp, NULL, 2);
            if (result9 != testValuesBinary64[i].value)
            printf("std::stoull binary64 unit test failed with \"%s\", returned %lld\n", testValuesBinary64[i].string, result9 );
        }
        catch (...) {
                // stoull throws when it encounters non-numeric data
        }

#if __cplusplus >= 201703
        int64_t resultA;
        auto lame_result = std::from_chars( testValuesBinary64[i].string, testValuesBinary64[i].string+strlen(testValuesBinary64[i].string)+1, resultA, 2 );
        if ((lame_result.ec == std::errc()) && resultA != testValuesBinary64[i].value)
            printf("std::from_chars binary64 unit test failed with \"%s\", returned %lld\n", testValuesBinary64[i].string, resultA );
#endif
    
    }




    // octal
    int32StringTest testValuesOctal32[] = {
        { 0, "0" },
        { 1, "1" },
        { 2, "2" },
        { 7, "7" },
        { 8, "10" },
        { 010, "010" },
        { 25, "31" },
        { 64, "100" },
        { 512, "1000" },
        { 585, "1111" },
        { 01111, "01111" },
        { 4096, "10000" },
        
        { 0, "8" }, // error case
        { 0, "9" }, // error case
        { 0, "a" }, // error case
        { 0, "f" }, // error case
        { 0, "z" }, // error case
        { 0, "ZZZ" }, // error case
        { 0, "We have very strict policies regarding eight-speech." }, // error case
    };
    
    size_t testCountOctal32 = sizeof(testValuesOctal32) / sizeof(testValuesOctal32[0]);
    
    for (size_t i = 0; i <testCountOctal32; ++i ) {
        
        long result2 = strtol( testValuesOctal32[i].string, NULL, 8 );
        if (result2 != testValuesOctal32[i].value)
            printf("strtol octal32 unit test failed with \"%s\", returned %ld\n", testValuesOctal32[i].string, result2 );
    
        long result3 = strtoul( testValuesOctal32[i].string, NULL, 8 );
        if (result3 != testValuesOctal32[i].value)
            printf("strtoul octal32 unit test failed with \"%s\", returned %lu\n", testValuesOctal32[i].string, result3 );

        long long result6 = strtoll( testValuesOctal32[i].string, NULL, 8 );
        if (result6 != testValuesOctal32[i].value)
            printf("strtoll octal32 unit test failed with \"%s\", returned %lld\n", testValuesOctal32[i].string, result6 );
        
        long long result7 = strtoull( testValuesOctal32[i].string, NULL, 8 );
        if (result7 != testValuesOctal32[i].value)
            printf("strtoull octal32 unit test failed with \"%s\", returned %llu\n", testValuesOctal32[i].string, result7 );

        int32_t result9 = simple_strtol( testValuesOctal32[i].string, NULL, 8 );
        if (result9 != testValuesOctal32[i].value)
            printf("simple_strtol octal32 unit test failed with \"%s\", returned %d\n", testValuesOctal32[i].string, result9 );
        
        std::string temp( testValuesOctal32[i].string );
        try {
            unsigned long result7 = std::stoi (temp, NULL, 8);
            if (result7 != testValuesOctal32[i].value)
            printf("std::stoi octal32 unit test failed with \"%s\", returned %ld\n", testValuesOctal32[i].string, result7 );
        }
        catch (...) {
                // stoi throws when it encounters non-numeric data
        }
        
        try {
            long result8 = std::stol (temp, NULL, 8);
            if (result8 != testValuesOctal32[i].value)
            printf("std::stol octal32 unit test failed with \"%s\", returned %ld\n", testValuesOctal32[i].string, result8 );
        }
        catch (...) {
                // stol throws when it encounters non-numeric data
        }
        
        try {
            unsigned long result8 = std::stoul (temp, NULL, 8);
            if (result8 != testValuesOctal32[i].value)
            printf("std::stoul octal32 unit test failed with \"%s\", returned %lu\n", testValuesOctal32[i].string, result8 );
        }
        catch (...) {
                // stoul throws when it encounters non-numeric data
        }
        
        try {
            long long result9 = std::stoll (temp, NULL, 8);
            if (result9 != testValuesOctal32[i].value)
            printf("std::stoll octal32 unit test failed with \"%s\", returned %lld\n", testValuesOctal32[i].string, result9 );
        }
        catch (...) {
                // stoll throws when it encounters non-numeric data
        }
        
        try {
            unsigned long long result9 = std::stoull (temp, NULL, 8);
            if (result9 != testValuesOctal32[i].value)
            printf("std::stoull octal32 unit test failed with \"%s\", returned %lld\n", testValuesOctal32[i].string, result9 );
        }
        catch (...) {
                // stoull throws when it encounters non-numeric data
        }

#if __cplusplus >= 201703
        long resultA;
        auto lame_result = std::from_chars( testValuesOctal32[i].string, testValuesOctal32[i].string+max_number_size, resultA, 8 );
        if ((lame_result.ec == std::errc()) && resultA != testValuesOctal32[i].value)
            printf("std::from_chars octal32 unit test failed with \"%s\", returned %ld\n", testValuesOctal32[i].string, resultA );
#endif
    
    }
    
    
    
    
    // octal
    int64StringTest testValuesOctal64[] = {
        { 0, "0" },
        { 1, "1" },
        { 1, "000000001" },
        { 2, "2" },
        { 7, "7" },
        { 8, "10" },
        { 010, "010" },
        { 00000010, "00000010" },
        { 25, "31" },
        { 64, "100" },
        { 512, "1000" },
        { 585, "1111" },
        { 01111, "01111" },
        { 4096, "10000" },
        { 0xffffffff, "37777777777" },
        { 0x1000000000, "1000000000000" },
        { 281474976710655, "7777777777777777" },
        { 0x7fffffffffffffff, "777777777777777777777" },
        
        { 0, "8" }, // error case
        { 0, "9" }, // error case
        { 0, "a" }, // error case
        { 0, "f" }, // error case
        { 0, "z" }, // error case
        { 0, "ZZZ" }, // error case
        { 0, "OCT 31 == DEC 25" }, // error case
    };
    
    size_t testCountOctal64 = sizeof(testValuesOctal64) / sizeof(testValuesOctal64[0]);
    
    for (size_t i = 0; i <testCountOctal64; ++i ) {
        
        if (sizeof(long) == 8) {
            long result2 = strtol( testValuesOctal64[i].string, NULL, 8 );
            if (result2 != testValuesOctal64[i].value)
                printf("strtol octal64 unit test failed with \"%s\", returned %ld\n", testValuesOctal64[i].string, result2 );
        
            long result3 = strtoul( testValuesOctal64[i].string, NULL, 8 );
            if (result3 != testValuesOctal64[i].value)
                printf("strtoul octal64 unit test failed with \"%s\", returned %lu\n", testValuesOctal64[i].string, result3 );
        }

        int64_t result9 = simple_strtol( testValuesOctal64[i].string, NULL, 8 );
        if (result9 != testValuesOctal64[i].value)
            printf("simple_strtol octal64 unit test failed with \"%s\", returned %lld\n", testValuesOctal64[i].string, result9 );

        long long result6 = strtoll( testValuesOctal64[i].string, NULL, 8 );
        if (result6 != testValuesOctal64[i].value)
            printf("strtoll octal64 unit test failed with \"%s\", returned %lld\n", testValuesOctal64[i].string, result6 );
        
        long long result7 = strtoull( testValuesOctal64[i].string, NULL, 8 );
        if (result7 != testValuesOctal64[i].value)
            printf("strtoull octal64 unit test failed with \"%s\", returned %llu\n", testValuesOctal64[i].string, result7 );
        
        std::string temp( testValuesOctal64[i].string );
        if (sizeof(long) == 8) {
            try {
                unsigned long result7 = std::stoi (temp, NULL, 8);
                if (result7 != testValuesOctal64[i].value)
                printf("std::stoi octal64 unit test failed with \"%s\", returned %ld\n", testValuesOctal64[i].string, result7 );
            }
            catch (...) {
                    // stoi throws when it encounters non-numeric data
            }
            
            try {
                long result8 = std::stol (temp, NULL, 8);
                if (result8 != testValuesOctal64[i].value)
                printf("std::stol octal64 unit test failed with \"%s\", returned %ld\n", testValuesOctal64[i].string, result8 );
            }
            catch (...) {
                    // stol throws when it encounters non-numeric data
            }
            
            try {
                unsigned long result8 = std::stoul (temp, NULL, 8);
                if (result8 != testValuesOctal64[i].value)
                printf("std::stoul octal64 unit test failed with \"%s\", returned %lu\n", testValuesOctal64[i].string, result8 );
            }
            catch (...) {
                    // stoul throws when it encounters non-numeric data
            }
        }
        
        try {
            long long result9 = std::stoll (temp, NULL, 8);
            if (result9 != testValuesOctal64[i].value)
            printf("std::stoll octal64 unit test failed with \"%s\", returned %lld\n", testValuesOctal64[i].string, result9 );
        }
        catch (...) {
                // stoll throws when it encounters non-numeric data
        }
        
        try {
            unsigned long long result9 = std::stoull (temp, NULL, 8);
            if (result9 != testValuesOctal64[i].value)
            printf("std::stoull octal64 unit test failed with \"%s\", returned %lld\n", testValuesOctal64[i].string, result9 );
        }
        catch (...) {
                // stoull throws when it encounters non-numeric data
        }

#if __cplusplus >= 201703
        int64_t resultA;
        auto lame_result = std::from_chars( testValuesOctal64[i].string, testValuesOctal64[i].string+max_number_size, resultA, 8 );
        if ((lame_result.ec == std::errc()) && resultA != testValuesOctal64[i].value)
            printf("std::from_chars octal64 unit test failed with \"%s\", returned %lld\n", testValuesOctal64[i].string, resultA );
#endif
    
    }


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
    
    
    UnitTest();
    
    CreateNumberStrings();


    testInteger();
    testHex();

    iterations /= 2;
    testFloat();
    testFloatSci();


    return 0;
}

/******************************************************************************/
/******************************************************************************/
