/*
    Copyright 2009-2011 Adobe Systems Incorporated
    Copyright 2018 Chris Cox
    Distributed under the MIT License (see accompanying file LICENSE_1_0_0.txt
    or a copy at http://stlab.adobe.com/licenses.html )


Goal: test the performance of various common ways of parsing a number from a string

Assumptions:
	
	1) atol, atoi, atof, strtol, strtof should all be faster than scanf, due to lower overhead
	
	2) strtof, strtol should be simlar in performance to atof, atol
	
	3) strtol and strtoul should have similar performance
	
	4) strtof and strtod should have similar performance
	
	5) library routines should be the same speed or faster than simple source versions of functions


MacOS 10.12.6
test   description   absolute   operations   ratio with
number               time       per second   test0

 0          "atol"   1.02 sec   24.43 M     1.00
 1          "atoi"   1.03 sec   24.26 M     1.01
 2        "strtol"   1.07 sec   23.44 M     1.04
 3       "strtoul"   0.98 sec   25.40 M     0.96
 4      "sscanf d"   3.96 sec    6.31 M     3.87
 5         "atoll"   1.01 sec   24.64 M     0.99
 6       "strtoll"   1.05 sec   23.81 M     1.03
 7      "strtoull"   1.03 sec   24.24 M     1.01
 8     "sscanf ll"   4.04 sec    6.18 M     3.95
 9 "simple_strtol"   0.47 sec   53.44 M     0.46


test       description   absolute   operations   ratio with
number                   time       per second   test0

 0        "strtol hex"   1.42 sec   17.62 M     1.00
 1       "strtoul hex"   1.31 sec   19.07 M     0.92
 2          "sscanf X"   4.49 sec    5.57 M     3.17
 3       "strtoll hex"   1.37 sec   18.20 M     0.97
 4      "strtoull hex"   1.35 sec   18.50 M     0.95
 5        "sscanf llX"   4.60 sec    5.44 M     3.24
 6 "simple_strtol hex"   0.39 sec   63.73 M     0.28


test     description   absolute   operations   ratio with
number                 time       per second   test0

 0            "atof"   1.67 sec   14.93 M     1.00
 1          "strtof"   3.67 sec    6.81 M     2.19
 2          "strtod"   1.68 sec   14.90 M     1.00
 3  "sscanf f float"   9.06 sec    2.76 M     5.41
 4  "sscanf g float"   9.06 sec    2.76 M     5.41
 5 "sscanf f double"   7.06 sec    3.54 M     4.22
 6 "sscanf g double"   7.04 sec    3.55 M     4.21
 7   "simple_strtod"   0.54 sec   46.73 M     0.32


test       description   absolute   operations   ratio with
number                   time       per second   test0

 0            "atof E"   1.65 sec   15.17 M     1.00
 1          "strtof E"   3.81 sec    6.56 M     2.31
 2          "strtod E"   1.64 sec   15.23 M     1.00
 3  "sscanf f float E"   9.52 sec    2.63 M     5.78
 4  "sscanf g float E"   9.53 sec    2.62 M     5.78
 5 "sscanf f double E"   7.23 sec    3.46 M     4.38
 6 "sscanf g double E"   7.24 sec    3.45 M     4.40
 7   "simple_strtod E"   0.60 sec   41.49 M     0.37

 
*/

#include <cstdio>
#include <ctime>
#include <climits>
#include <cerrno>
#include "benchmark_stdint.hpp"
#include "benchmark_timer.h"
#include "benchmark_results.h"

/******************************************************************************/

int iterations = 50000;

#define SIZE 	500

const int max_number_size = 20;

/******************************************************************************/

char integer_strings[ SIZE ][ max_number_size ];
char hex_strings[ SIZE ][ max_number_size ];
char float_strings[ SIZE ][ max_number_size ];
char float_stringsE[ SIZE ][ max_number_size ];

long global_integer_sum = 0;
unsigned long global_uinteger_sum = 0;
int64_t global_64_sum = 0;
uint64_t global_u64_sum = 0;
float global_float_sum = 0.0;
double global_double_sum = 0.0;

#if WIN32
#define snprintf	_snprintf
#define atoll	_atoi64
#endif

void CreateNumberStrings() {
	long i;
	
	global_integer_sum = 0;
	global_64_sum = 0;
	for (i = 0; i < SIZE; ++i)
		{
		int value = i * 477;
		snprintf( integer_strings[i], max_number_size-1, "%d", value );
		snprintf( hex_strings[i], max_number_size-1, "0x%X", value );
		global_uinteger_sum += (unsigned long)value;
		global_integer_sum += (long)value;
		global_u64_sum += (uint64_t)value;
		global_64_sum += (int64_t)value;
		}
	
	global_float_sum = 0.0;
	global_double_sum = 0.0;
	for (i = 0; i < SIZE; ++i)
		{
		double value = i * 47.25318;
		snprintf( float_strings[i], max_number_size-1, "%f", value );
		snprintf( float_stringsE[i], max_number_size-1, "%e", value );
		global_float_sum += (float)value;
		global_double_sum += value;
		}
}

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
		printf("test %i failed (%lld, %llu)\n", current_test, result, global_u64_sum);
}

/******************************************************************************/

inline void check_sum64(uint64_t result) {
	if (result != global_u64_sum)
		printf("test %i failed (%llu, %llu)\n", current_test, result, global_u64_sum);
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


inline bool quick_isspace( int value )
    { return (value == ' '); }

inline bool quick_isdigit( int value )
    { return ((value <= '9') && (value >= '0')); }

//inline bool quick_isoctal( int value )
//    { return ((value <= '7') && (value >= '0')); }

//inline bool quick_isxdigit( int value )
//    { return ( ((value <= '9') && (value >= '0')) || ((value <= 'f') && (value >='a')) || ((value <= 'F') && (value >='A')) ); }

const static double powersOf10[] = {	10.0,
										1.0e2,
										1.0e4,
										1.0e8,
										1.0e16,
										1.0e32,
										1.0e64,
										1.0e128,
										1.0e256
									};

static double
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
//		But BSD and other open sources I can find don't implement hex!?
// 0x1.fffffep+127f
// 0xb.17217f7d1cf79acp-4L
// printf %A %a

	
	// handle integer portion or special cases INFINITY, NAN
	// potentially large number of digits, could unroll
    if (quick_isdigit(*p)) {
		result = (double)(*p++ - '0');

		while (*p && quick_isdigit(*p)) {
			result = result * 10.0 + (double)(*p - '0');
			++p;
		}
		lastp = p;
	
// TODO - overrange detection?

	}
	else if (*p == 'I' && (strcmp(p+1,"NFINITY") == 0)) {
		result = INFINITY;
		lastp = p+8;
		goto done;
	}
	else if (*p == 'N' && p[1] == 'A' && p[2] == 'N') {	// 3 compares faster than calling strcmp
		result = NAN;
		lastp = p+3;
		goto done;
	}


    // handle decimal and fraction
    if (*p == '.') {
		double fraction = 0.0;
		double base = 0.1;
		
		++p;

		// potentially large number of digits, could unroll
		while (p[0] && p[1] && quick_isdigit(p[0]) && quick_isdigit(p[1])) {
			fraction += base * (p[0] - '0');
			fraction += base * 0.1 * (p[1] - '0');
			base *= 0.01;	//	base /= 100.0;
			p+=2;
		}
		while (*p && quick_isdigit(*p)) {
			fraction += base * (*p - '0');
			base *= 0.1;	//	base /= 10.0;
			++p;
		}
		
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
			while (*p == '0')
				++p;
			
			// max 3 digits, could unroll
			e = (int)(*p++ - '0');
			while (*p && quick_isdigit(*p)) {
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
				
				// this could be unrolled
				for (int j = 0; j < 9; ++j) {	// max 308 (9 bits) for double, max 38 (6 bits) for float
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
    else if (p > nptr && !quick_isdigit(*(p-1))) {	// did we end on a bad character?
		lastp = nptr;
		//goto done;
	}


done:
    if (endptr)
		*endptr = (char*)lastp;
	
    return result;
}

/******************************************************************************/

static long
simple_strtol(const char *str, char **endptr, int base)
{
    long result = 0;
	bool sign_negative = false;
    const char *lastp = str;
    const char *p = str;
	
	static long division_table[ 37 ];	// index 0,1 never used, but cleaner to index directly
	static unsigned char hex_table[ 256 ];	// actually used for all uncommon bases
	static bool hex_table_initialized = false;
	
	
    if ( base != 0 && (base < 2 || base > 36) )
		return 0;
	
	// setup cached tables - could also be hard coded, or defined by locale
	if (!hex_table_initialized) {
		memset(hex_table,255,sizeof(hex_table));
		memset(division_table,0,sizeof(division_table));
		for ( long i = '0'; i <= '9'; ++i)
			hex_table[i] = i - '0';
		for ( long j = 'A'; j <= 'Z'; ++j)
			hex_table[j] = 10 + j - 'A';
		for ( long k = 'a'; k <= 'z'; ++k)
			hex_table[k] = 10 + k - 'a';
		hex_table_initialized = true;
	}

    while (quick_isspace(*p))
		p++;
	
	
    if (*p == '-')
		{
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
		else if (base == 0) {	// octal special case
			p++;
			base = 8;
		}
	}
	if (base == 0)	// special case did not see hex or octal prefix, so becomes base 10
		base = 10;
	
	
	if (base == 10) {	// this is about the same speed as the generic loop below, but should offer optimization opportunities

		if (quick_isdigit(*p)) {
			const long upper_limit = LONG_MAX/10;
			result = (long)(*p++ - '0');

			// unrolling still has too many dependent branches, did not make a noticeable performance improvement.
			while (*p && quick_isdigit(*p)) {
				long digit = (long)(*p - '0');
				if (result > upper_limit) { errno = ERANGE; result = LONG_MAX; break; }
				result *= 10;
				if ((LONG_MAX - result) < digit) { errno = ERANGE; result = LONG_MAX; break; }	// has to be faster way to check 1.02s vs 0.86s
				result += digit;
				++p;
			}
			lastp = p;
		}

	}
	else {
		// NOTE - this is faster for base=16, because ishexdigit tests take time
		
		if (division_table[base] == 0)
			division_table[base] = LONG_MAX / base;
		
		const long upper_limit = division_table[base];
		
		// unrolling still has too many dependent branches, did not make a noticeable performance improvement.
		result = 0;
		while (*p) {
			long digit = hex_table[*p];
			if (digit >= base)
				break;
			if (result > upper_limit) { errno = ERANGE; result = LONG_MAX; break; }
			result *= base;
			if ((LONG_MAX - result) < digit) { errno = ERANGE; result = LONG_MAX; break; }
			result += digit;
			++p;
		}
		lastp = p;
	}

done:
    if (endptr)
		*endptr = (char*)lastp;
	
    return ((sign_negative) ? -result : result);
}


/******************************************************************************/

int main (int argc, char *argv[])
{
	
	// output command for documentation:
	int i, j;
	for (i = 0; i < argc; ++i)
		printf("%s ", argv[i] );
	printf("\n");

	if (argc > 1) iterations = atoi(argv[1]);
	
	
	CreateNumberStrings();
	

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
	

#if !WIN32
// does not exist in MSDEV
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
#endif
	
#if !WIN32
// does not exist in MSDEV
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
#endif
	
	
	start_timer();
	for (i = 0; i != iterations; ++i)
		{
		int64_t sum = 0;
		for (j = 0; j < SIZE; ++j )
			{
			int64_t result;
			sscanf( integer_strings[j], "%lld", &result );
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
			sum += simple_strtol ( integer_strings[j], NULL, 0 );
			}
		check_sum(sum);
		}
	record_result( timer(), "simple_strtol");
	
	
	summarize("atol", SIZE, iterations, kDontShowGMeans, kDontShowPenalty );
	
	
	
	
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


#if !WIN32
// does not exist in MSDEV
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
#endif
	
#if !WIN32
// does not exist in MSDEV
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
#endif
	
	
	start_timer();
	for (i = 0; i != iterations; ++i)
		{
		int64_t sum = 0;
		for (j = 0; j < SIZE; ++j )
			{
			int64_t result;
			sscanf( hex_strings[j], "%llX", &result );
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
			sum += simple_strtol ( hex_strings[j], NULL, 16 );
			}
		check_sum(sum);
		}
	record_result( timer(), "simple_strtol hex");
	
	summarize("atol hex", SIZE, iterations, kDontShowGMeans, kDontShowPenalty );
	
	
	
	
	start_timer();
	for (i = 0; i != iterations; ++i)
		{
		float sum = 0.0;
		for (j = 0; j < SIZE; ++j )
			{
			sum += atof ( float_strings[j] );
			}
		check_sum_float(sum);
		}
	record_result( timer(), "atof");
	
#if !WIN32
// does not exist in MSDEV
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
#endif

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
		double sum = 0.0;
		for (j = 0; j < SIZE; ++j )
			{
			sum += simple_strtod ( float_strings[j], NULL );
			}
		check_sum_double(sum);
		}
	record_result( timer(), "simple_strtod");

	
	summarize("atof", SIZE, iterations, kDontShowGMeans, kDontShowPenalty );

	

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
	
#if !WIN32
// does not exist in MSDEV
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
#endif

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
		double sum = 0.0;
		for (j = 0; j < SIZE; ++j )
			{
			sum += simple_strtod ( float_stringsE[j], NULL );
			}
		check_sum_double(sum);
		}
	record_result( timer(), "simple_strtod E");

	
	summarize("atof E", SIZE, iterations, kDontShowGMeans, kDontShowPenalty );


	return 0;
}

