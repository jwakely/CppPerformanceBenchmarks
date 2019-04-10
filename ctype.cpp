/*
    Copyright 2008-2010 Adobe Systems Incorporated
    Copyright 2018 Chris Cox
    Distributed under the MIT License (see accompanying file LICENSE_1_0_0.txt
    or a copy at http://stlab.adobe.com/licenses.html )


Goal:  Test performance of the ctype, tolower/toupper, and similar functions

Assumptions:
    1) None of the ctype routines should be slower than a table lookup and bitfield comparison
    
    2) None of the ctype routines should be slower than a simple range comparison, where applicable
    
    3) tolower and toupper should not be slower than table lookups (assuming locale data is cached)
    
    4) isascii should be as fast as an inline calculation

    5) The choice of <ctype.h> or <cctype> should have no impact on performance (can't test with single source file)
 
    6) ctype like functions that have too many comparisons, should be reduced to precalculated lookup tables.
        This is important for parsers/tokenizers.
        NOTE - it looks like around 6-8 comparisons is the magic point to move to a table.



NOTE -  isdigit and isxdigit are locale dependent
        inline versions do not work with locales
        table versions mimic use of locales


TODO - wctype.h - iswascii, iswdigit, iswblank, etc.

*/

#include "benchmark_stdint.hpp"
#include <cstddef>
#include <cstdio>
#include <ctime>
#include <cstdlib>
#include <cmath>
#include "benchmark_results.h"
#include "benchmark_algorithms.h"
#include "benchmark_timer.h"

//#include <ctype.h>        // may be faster depending on macros/inlines
#include <cctype>


// various semi-current flavors of BSD
#if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
#define isBSD   1
#endif

// one of these should be defined on Linux derived OSes
#if defined(_LINUX_TYPES_H) || defined(_SYS_TYPES_H)
#define isLinux 1
#endif

/******************************************************************************/

// this constant may need to be adjusted to give reasonable minimum times
// For best results, times should be about 1.0 seconds for the minimum test run
int iterations = 150000;

// 8k of data, needs to be a multiple of 256 to cover all char values
// this is intended to remain within the L2 cache of most common CPUs
const int SIZE = 8192;

/******************************************************************************/

// TODO - ccox - move these to benchmark_shared_tests.h
template <typename T>
inline void check_expected_sum(T result, T expected) {
  if (result != expected)
    printf("test %i failed\n", current_test);
}

/******************************************************************************/

volatile uint64_t gFake = 0;

template <typename T>
inline void check_expected_fake(T result) {
    gFake += result;
}

/******************************************************************************/

template <typename Iterator, typename T>
void fill_increasing(Iterator first, Iterator last, T value) {
    while (first != last) *first++ = value++;
}

/******************************************************************************/

template <typename T, typename Shifter>
void test_bool_expected(T* first, int count, int expected, const char *label) {
  int i;
  
  start_timer();
  
  for(i = 0; i < iterations; ++i) {
    int result = 0;
    for (int n = 0; n < count; ++n) {
        result += int( Shifter::do_shift( first[n] ) );
    }
    check_expected_sum<int>(result, expected);
  }
  
  record_result( timer(), label );
}

/******************************************************************************/

template <typename T, typename Shifter>
void test_bool_faked(T* first, int count, const char *label) {
  int i;
  
  start_timer();
  
  for(i = 0; i < iterations; ++i) {
    int result = 0;
    for (int n = 0; n < count; ++n) {
        result += int( Shifter::do_shift( first[n] ) );
    }
    check_expected_fake<int>(result);
  }
  
  record_result( timer(), label );
}

/******************************************************************************/
/******************************************************************************/

uint8_t tolower_table[256];
uint8_t toupper_table[256];

// global table of character flags
uint16_t char_type_table[256];

const uint16_t kDigitFlag = 0x0001;
const uint16_t kASCIIFlag = 0x0002;
const uint16_t kUpperFlag = 0x0004;
const uint16_t kLowerFlag = 0x0008;
const uint16_t kBlankFlag = 0x0010;
const uint16_t kCntrlFlag = 0x0020;
const uint16_t kGraphFlag = 0x0040;
const uint16_t kPrintFlag = 0x0080;
const uint16_t kPunctFlag = 0x0100;
const uint16_t kSpaceFlag = 0x0200;
const uint16_t kXDigitFlag = 0x0400;

//const uint16_t kAlphaFlag = kUpperFlag | kLowerFlag;
//const uint16_t kAlphaNumFlag = kAlphaFlag | kDigitFlag;

void init_char_types_table() {
    int i;
    
    for (i = 0; i < 256; ++i) {
        uint16_t result = 0;
        
        if (isdigit(i)) result |= kDigitFlag;
        if (isascii(i)) result |= kASCIIFlag;
        if (isupper(i)) result |= kUpperFlag;
        if (islower(i)) result |= kLowerFlag;
#if !WIN32
        // missing on MSVC
        if (isblank(i)) result |= kBlankFlag;
#endif
        if (iscntrl(i)) result |= kCntrlFlag;
        if (isgraph(i)) result |= kGraphFlag;
        if (isprint(i)) result |= kPrintFlag;
        if (ispunct(i)) result |= kPunctFlag;
        if (isspace(i)) result |= kSpaceFlag;
        if (isxdigit(i)) result |= kXDigitFlag;
        
        char_type_table[i] = result;
        
        tolower_table[i] = tolower(i);
        toupper_table[i] = toupper(i);
    }

}

/******************************************************************************/

inline bool cheap_isdigit( int value )
    { return ((value <= '9') && (value >= '0')); }

inline bool cheap_isascii( int value )
    { return ((value & ~0x7f) == 0); }

inline bool cheap_isascii2( int value )
    { return (value >= 0 && value <= 127); }

inline bool table_isdigit( int value )
    { return (value >= 0 && value <= 255 && (char_type_table[value] & kDigitFlag) == kDigitFlag); }

inline bool table_isascii( int value )
    { return (value >= 0 && value <= 255 && (char_type_table[value] & kASCIIFlag) == kASCIIFlag); }

inline bool table_isascii2( int value )
    { if (value < 0 || value > 255) return false;
      return ((char_type_table[value] & kASCIIFlag) != 0); }

inline int table_tolower( int value )
    { if (value < 0 || value > 255 || value == EOF) return value;
      return tolower_table[ value ]; }

inline int table_toupper( int value )
    { if (value < 0 || value > 255 || value == EOF) return value;
      return toupper_table[ value ]; }

/******************************************************************************/
/******************************************************************************/

template <typename T>
    struct ctype_isdigit {
      static bool do_shift(T input) { return (isdigit(input)); }
    };

template <typename T>
    struct ctype_cheap_isdigit {
      static bool do_shift(T input) { return (cheap_isdigit(input)); }
    };

template <typename T>
    struct ctype_table_isdigit {
      static bool do_shift(T input) { return (table_isdigit(input)); }
    };

template <typename T>
    struct ctype_isalnum {
      static bool do_shift(T input) { return (isalnum(input)); }
    };

template <typename T>
    struct ctype_isalpha {
      static bool do_shift(T input) { return (isalpha(input)); }
    };

#if !WIN32
// function missing in MSVC
template <typename T>
    struct ctype_isblank {
      static bool do_shift(T input) { return (isblank(input)); }
    };
#endif

template <typename T>
    struct ctype_iscntrl {
      static bool do_shift(T input) { return (iscntrl(input)); }
    };

template <typename T>
    struct ctype_isgraph {
      static bool do_shift(T input) { return (isgraph(input)); }
    };

template <typename T>
    struct ctype_islower {
      static bool do_shift(T input) { return (islower(input)); }
    };

template <typename T>
    struct ctype_isprint {
      static bool do_shift(T input) { return (isprint(input)); }
    };

template <typename T>
    struct ctype_ispunct {
      static bool do_shift(T input) { return (ispunct(input)); }
    };

template <typename T>
    struct ctype_isspace {
      static bool do_shift(T input) { return (isspace(input)); }
    };

template <typename T>
    struct ctype_isupper {
      static bool do_shift(T input) { return (isupper(input)); }
    };

template <typename T>
    struct ctype_isxdigit {
      static bool do_shift(T input) { return (isxdigit(input)); }
    };

template <typename T>
    struct ctype_isascii {
      static bool do_shift(T input) { return (isascii(input)); }
    };

template <typename T>
    struct ctype_cheap_isascii {
      static bool do_shift(T input) { return (cheap_isascii(input)); }
    };

template <typename T>
    struct ctype_cheap_isascii2 {
      static bool do_shift(T input) { return (cheap_isascii2(input)); }
    };

template <typename T>
    struct ctype_table_isascii {
      static bool do_shift(T input) { return (table_isascii(input)); }
    };

template <typename T>
    struct ctype_table_isascii2 {
      static bool do_shift(T input) { return (table_isascii2(input)); }
    };

template <typename T>
    struct ctype_tolower {
      static int do_shift(T input) { return (tolower(input)); }
    };

template <typename T>
    struct ctype_toupper {
      static int do_shift(T input) { return (toupper(input)); }
    };

template <typename T>
    struct ctype_table_tolower {
      static int do_shift(T input) { return (table_tolower(input)); }
    };

template <typename T>
    struct ctype_table_toupper {
      static int do_shift(T input) { return (table_toupper(input)); }
    };

/******************************************************************************/

// missing on Linux and Windows
// present in Mach
#if !isLinux && !WIN32 && !defined(isBSD)

template <typename T>
    struct ctype_isnumber {
      static bool do_shift(T input) { return (isnumber(input)); }
    };

template <typename T>
    struct ctype_ishexnumber{
      static bool do_shift(T input) { return (ishexnumber(input)); }
    };

template <typename T>
    struct ctype_isideogram{
      static bool do_shift(T input) { return (isideogram(input)); }
    };

template <typename T>
    struct ctype_isphonogram{
      static bool do_shift(T input) { return (isphonogram(input)); }
    };

template <typename T>
    struct ctype_isrune{
      static bool do_shift(T input) { return (isrune(input)); }
    };

template <typename T>
    struct ctype_isspecial{
      static bool do_shift(T input) { return (isspecial(input)); }
    };

#endif

/******************************************************************************/
/******************************************************************************/

// NOTE - I found functions similar to this used on critical paths in a couple of databases and parsers

template <typename T>
struct ctype_cheap_isidentifier_long {
      static bool do_shift(T c) { return (c >= '0' && c <= '9')
                                        || (c >= 'a' && c <= 'z')
                                        || (c >= 'A' && c <= 'Z')
                                        || c == '@' || c == '#'
                                        || c == '$' || c == '~'
                                        || c == '-' || c == '+'
                                        || c == '.' || c == '_'
                                        || c == '&'; };
};

/******************************************************************************/

template <typename T>
struct ctype_cheap_isidentifier2 {
      static bool do_shift(T c) { return (c >= 'a' && c <= 'z'); }
};

template <typename T>
    struct ctype_cheap_isidentifier4 {
      static bool do_shift(T c) { return (c >= '0' && c <= '9')
                                        || (c >= 'a' && c <= 'z'); }
};

template <typename T>
struct ctype_cheap_isidentifier6 {
      static bool do_shift(T c) { return (c >= '0' && c <= '9')
                                        || (c >= 'a' && c <= 'z')
                                        || (c >= 'A' && c <= 'Z'); }
};

template <typename T>
struct ctype_cheap_isidentifier8 {
      static bool do_shift(T c) { return (c >= '0' && c <= '9')
                                        || (c >= 'a' && c <= 'z')
                                        || (c >= 'A' && c <= 'Z')
                                        || c == '$' || c == '~'; }
};

template <typename T>
struct ctype_cheap_isidentifier10 {
      static bool do_shift(T c) { return (c >= '0' && c <= '9')
                                        || (c >= 'a' && c <= 'z')
                                        || (c >= 'A' && c <= 'Z')
                                        || c == '@' || c == '#'
                                        || c == '$' || c == '~'; }
};

template <typename T>
struct ctype_cheap_isidentifier12 {
      static bool do_shift(T c) { return (c >= '0' && c <= '9')
                                        || (c >= 'a' && c <= 'z')
                                        || (c >= 'A' && c <= 'Z')
                                        || c == '@' || c == '#'
                                        || c == '$' || c == '~'
                                        || c == '-' || c == '+'; }
};

template <typename T>
struct ctype_cheap_isidentifier14 {
      static bool do_shift(T c) { return (c >= '0' && c <= '9')
                                        || (c >= 'a' && c <= 'z')
                                        || (c >= 'A' && c <= 'Z')
                                        || c == '@' || c == '#'
                                        || c == '$' || c == '~'
                                        || c == '-' || c == '+'
                                        || c == '.' || c == '_'; }
};

template <typename T>
struct ctype_cheap_isidentifier16 {
      static bool do_shift(T c) { return (c >= '0' && c <= '9')
                                        || (c >= 'a' && c <= 'z')
                                        || (c >= 'A' && c <= 'Z')
                                        || c == '@' || c == '#'
                                        || c == '$' || c == '~'
                                        || c == '-' || c == '+'
                                        || c == '.' || c == '_'
                                        || c == '&' || c == ']'; }
};

template <typename T>
struct ctype_cheap_isidentifier18 {
      static bool do_shift(T c) { return (c >= '0' && c <= '9')
                                        || (c >= 'a' && c <= 'z')
                                        || (c >= 'A' && c <= 'Z')
                                        || c == '@' || c == '#'
                                        || c == '$' || c == '~'
                                        || c == '-' || c == '+'
                                        || c == '.' || c == '_'
                                        || c == '&' || c == ']'
                                        || c == ';' || c == '>'; }
};

template <typename T>
struct ctype_cheap_isidentifier20 {
      static bool do_shift(T c) { return (c >= '0' && c <= '9')
                                        || (c >= 'a' && c <= 'z')
                                        || (c >= 'A' && c <= 'Z')
                                        || c == '@' || c == '#'
                                        || c == '$' || c == '~'
                                        || c == '-' || c == '+'
                                        || c == '.' || c == '_'
                                        || c == '&' || c == ']'
                                        || c == ';' || c == '>'
                                        || c == '\\' || c == '('; }
};

/******************************************************************************/

// ok, some people write their character tests in bizarre ways
template <typename T>
struct ctype_cheap_isidentifier2a {
      static bool do_shift(T c) { if (c >= 'a' && c <= 'z')  return true;
                                  else return false; }
};

template <typename T>
    struct ctype_cheap_isidentifier4a {
      static bool do_shift(T c) { if (c >= '0' && c <= '9') return true;
                                  if (c >= 'a' && c <= 'z') return true;
                                  return false; }
};

template <typename T>
struct ctype_cheap_isidentifier6a {
      static bool do_shift(T c) { if (c >= '0' && c <= '9') return true;
                                  if (c >= 'a' && c <= 'z') return true;
                                  if (c >= 'A' && c <= 'Z') return true;
                                  return false; }
};

template <typename T>
struct ctype_cheap_isidentifier8a {
      static bool do_shift(T c) { if (c >= '0' && c <= '9') return true;
                                  if (c >= 'a' && c <= 'z') return true;
                                  if (c >= 'A' && c <= 'Z') return true;
                                  if (c == '$' || c == '~') return true;
                                  return false; }
};

template <typename T>
struct ctype_cheap_isidentifier10a {
      static bool do_shift(T c) { if (c >= '0' && c <= '9') return true;
                                  if (c >= 'a' && c <= 'z') return true;
                                  if (c >= 'A' && c <= 'Z') return true;
                                  if (c == '@' || c == '#') return true;
                                  if (c == '$' || c == '~') return true;
                                  return false; }
};

template <typename T>
struct ctype_cheap_isidentifier12a {
      static bool do_shift(T c) { if (c >= '0' && c <= '9') return true;
                                  if (c >= 'a' && c <= 'z') return true;
                                  if (c >= 'A' && c <= 'Z') return true;
                                  if (c == '@' || c == '#') return true;
                                  if (c == '$' || c == '~') return true;
                                  if (c == '-' || c == '+') return true;
                                  return false;  }
};

template <typename T>
struct ctype_cheap_isidentifier14a {
      static bool do_shift(T c) { if (c >= '0' && c <= '9') return true;
                                  if (c >= 'a' && c <= 'z') return true;
                                  if (c >= 'A' && c <= 'Z') return true;
                                  if (c == '@' || c == '#') return true;
                                  if (c == '$' || c == '~') return true;
                                  if (c == '-' || c == '+') return true;
                                  if (c == '.' || c == '_') return true;
                                  return false; }
};

template <typename T>
struct ctype_cheap_isidentifier16a {
      static bool do_shift(T c) { if (c >= '0' && c <= '9') return true;
                                  if (c >= 'a' && c <= 'z') return true;
                                  if (c >= 'A' && c <= 'Z') return true;
                                  if (c == '@' || c == '#') return true;
                                  if (c == '$' || c == '~') return true;
                                  if (c == '-' || c == '+') return true;
                                  if (c == '.' || c == '_') return true;
                                  if (c == '&' || c == ']') return true;
                                  return false; }
};

template <typename T>
struct ctype_cheap_isidentifier18a {
      static bool do_shift(T c) { if (c >= '0' && c <= '9') return true;
                                  if (c >= 'a' && c <= 'z') return true;
                                  if (c >= 'A' && c <= 'Z') return true;
                                  if (c == '@' || c == '#') return true;
                                  if (c == '$' || c == '~') return true;
                                  if (c == '-' || c == '+') return true;
                                  if (c == '.' || c == '_') return true;
                                  if (c == '&' || c == ']') return true;
                                  if (c == ';' || c == '>') return true;
                                  return false; }
};

template <typename T>
struct ctype_cheap_isidentifier20a {
      static bool do_shift(T c) { if (c >= '0' && c <= '9') return true;
                                  if (c >= 'a' && c <= 'z') return true;
                                  if (c >= 'A' && c <= 'Z') return true;
                                  if (c == '@' || c == '#') return true;
                                  if (c == '$' || c == '~') return true;
                                  if (c == '-' || c == '+') return true;
                                  if (c == '.' || c == '_') return true;
                                  if (c == '&' || c == ']') return true;
                                  if (c == ';' || c == '>') return true;
                                  if (c == '\\' || c == '(') return true;
                                  return false; }
};

/******************************************************************************/
/******************************************************************************/

// expected counts for various character types
// for error checking
const int kExpected_isdigit = 10;
const int kExpected_isalnum = 2*26+10;
const int kExpected_isalpha = 2*26;
const int kExpected_isblank = 2;
const int kExpected_iscntrl = 33;
const int kExpected_isgraph = 94;
const int kExpected_islower = 26;
const int kExpected_isprint = 95;
const int kExpected_ispunct = 32;
const int kExpected_isspace = 6;
const int kExpected_isupper = 26;
const int kExpected_isxdigit = 22;
const int kExpected_isascii = 128;
const int kExpected_tolower = 33472;
const int kExpected_toupper = 31808;
const int kExpected_isideogram = 0;
const int kExpected_isphonogram = 0;
const int kExpected_isrune = 128;
const int kExpected_isspecial = 0;

/******************************************************************************/
/******************************************************************************/

int main(int argc, char** argv) {
    
    // output command for documentation:
    int i;
    for (i = 0; i < argc; ++i)
        printf("%s ", argv[i] );
    printf("\n");

    if (argc > 1) iterations = atoi(argv[1]);



    // our array of numbers to be operated upon
    uint8_t data[SIZE];

    // seed the random number generator, so we get repeatable results
    srand( iterations + 123 );
    
    init_char_types_table();

    // fill with several copies of the 256 possible values
    ::fill_increasing( data, data+SIZE, uint8_t(0) );
    // randomize the order
    random_shuffle( data, data+SIZE );



    test_bool_expected<uint8_t, ctype_isdigit<uint8_t> >(data,SIZE, kExpected_isdigit*(SIZE/256), "uint8_t isdigit");
    test_bool_expected<uint8_t, ctype_cheap_isdigit<uint8_t> >(data,SIZE, kExpected_isdigit*(SIZE/256), "uint8_t inline isdigit");
    test_bool_expected<uint8_t, ctype_table_isdigit<uint8_t> >(data,SIZE, kExpected_isdigit*(SIZE/256), "uint8_t table isdigit");
    test_bool_expected<uint8_t, ctype_isascii<uint8_t> >(data,SIZE, kExpected_isascii*(SIZE/256), "uint8_t isascii");
    test_bool_expected<uint8_t, ctype_cheap_isascii<uint8_t> >(data,SIZE, kExpected_isascii*(SIZE/256), "uint8_t inline isascii");
    test_bool_expected<uint8_t, ctype_cheap_isascii2<uint8_t> >(data,SIZE, kExpected_isascii*(SIZE/256), "uint8_t inline isascii2");
    test_bool_expected<uint8_t, ctype_table_isascii<uint8_t> >(data,SIZE, kExpected_isascii*(SIZE/256), "uint8_t table isascii");
    test_bool_expected<uint8_t, ctype_table_isascii2<uint8_t> >(data,SIZE, kExpected_isascii*(SIZE/256), "uint8_t table isascii2");

    test_bool_expected<uint8_t, ctype_isalnum<uint8_t> >(data,SIZE, kExpected_isalnum*(SIZE/256), "uint8_t isalnum");
    test_bool_expected<uint8_t, ctype_isalpha<uint8_t> >(data,SIZE, kExpected_isalpha*(SIZE/256), "uint8_t isalpha");
#if !WIN32
// function missing under MSVC
    test_bool_expected<uint8_t, ctype_isblank<uint8_t> >(data,SIZE, kExpected_isblank*(SIZE/256), "uint8_t isblank");
#endif
    test_bool_expected<uint8_t, ctype_iscntrl<uint8_t> >(data,SIZE, kExpected_iscntrl*(SIZE/256), "uint8_t iscntrl");
    test_bool_expected<uint8_t, ctype_isgraph<uint8_t> >(data,SIZE, kExpected_isgraph*(SIZE/256), "uint8_t isgraph");
    test_bool_expected<uint8_t, ctype_islower<uint8_t> >(data,SIZE, kExpected_islower*(SIZE/256), "uint8_t islower");
    test_bool_expected<uint8_t, ctype_isprint<uint8_t> >(data,SIZE, kExpected_isprint*(SIZE/256), "uint8_t isprint");
    test_bool_expected<uint8_t, ctype_ispunct<uint8_t> >(data,SIZE, kExpected_ispunct*(SIZE/256), "uint8_t ispunct");
    test_bool_expected<uint8_t, ctype_isspace<uint8_t> >(data,SIZE, kExpected_isspace*(SIZE/256), "uint8_t isspace");
    test_bool_expected<uint8_t, ctype_isupper<uint8_t> >(data,SIZE, kExpected_isupper*(SIZE/256), "uint8_t isupper");
    test_bool_expected<uint8_t, ctype_isxdigit<uint8_t> >(data,SIZE, kExpected_isxdigit*(SIZE/256), "uint8_t isxdigit");

// missing on Linux and Windows
// present in Mach
#if !isLinux && !WIN32 && !isBSD
    test_bool_expected<uint8_t, ctype_ishexnumber<uint8_t> >(data,SIZE, kExpected_isxdigit*(SIZE/256), "uint8_t ishexnumber");
    test_bool_expected<uint8_t, ctype_isnumber<uint8_t> >(data,SIZE, kExpected_isdigit*(SIZE/256), "uint8_t isnumber");
    test_bool_expected<uint8_t, ctype_isideogram<uint8_t> >(data,SIZE, kExpected_isideogram*(SIZE/256), "uint8_t isideogram");
    test_bool_expected<uint8_t, ctype_isphonogram<uint8_t> >(data,SIZE, kExpected_isphonogram*(SIZE/256), "uint8_t isphonogram");
    test_bool_expected<uint8_t, ctype_isrune<uint8_t> >(data,SIZE, kExpected_isrune*(SIZE/256), "uint8_t isrune");
    test_bool_expected<uint8_t, ctype_isspecial<uint8_t> >(data,SIZE, kExpected_isspecial*(SIZE/256), "uint8_t isspecial");
#endif

    summarize("uint8_t ctype functions", SIZE, iterations, kDontShowGMeans, kDontShowPenalty );


    test_bool_expected<uint8_t, ctype_tolower<uint8_t> >(data,SIZE, kExpected_tolower*(SIZE/256), "uint8_t tolower");
    test_bool_expected<uint8_t, ctype_toupper<uint8_t> >(data,SIZE, kExpected_toupper*(SIZE/256), "uint8_t toupper");
    test_bool_expected<uint8_t, ctype_table_tolower<uint8_t> >(data,SIZE, kExpected_tolower*(SIZE/256), "uint8_t table tolower");
    test_bool_expected<uint8_t, ctype_table_toupper<uint8_t> >(data,SIZE, kExpected_toupper*(SIZE/256), "uint8_t table toupper");

    summarize("uint8_t ctype toupper", SIZE, iterations, kDontShowGMeans, kDontShowPenalty );



    test_bool_faked<uint8_t, ctype_table_toupper<uint8_t> >(data,SIZE, "uint8_t ctype table lookup");
    test_bool_faked<uint8_t, ctype_cheap_isidentifier_long<uint8_t> >(data,SIZE, "uint8_t isIdentifier longform");
    test_bool_faked<uint8_t, ctype_cheap_isidentifier2<uint8_t> >(data,SIZE, "uint8_t isIdentifier 2");
    test_bool_faked<uint8_t, ctype_cheap_isidentifier4<uint8_t> >(data,SIZE, "uint8_t isIdentifier 4");
    test_bool_faked<uint8_t, ctype_cheap_isidentifier6<uint8_t> >(data,SIZE, "uint8_t isIdentifier 6");
    test_bool_faked<uint8_t, ctype_cheap_isidentifier8<uint8_t> >(data,SIZE, "uint8_t isIdentifier 8");
    test_bool_faked<uint8_t, ctype_cheap_isidentifier10<uint8_t> >(data,SIZE, "uint8_t isIdentifier 10");
    test_bool_faked<uint8_t, ctype_cheap_isidentifier12<uint8_t> >(data,SIZE, "uint8_t isIdentifier 12");
    test_bool_faked<uint8_t, ctype_cheap_isidentifier14<uint8_t> >(data,SIZE, "uint8_t isIdentifier 14");
    test_bool_faked<uint8_t, ctype_cheap_isidentifier16<uint8_t> >(data,SIZE, "uint8_t isIdentifier 16");
    test_bool_faked<uint8_t, ctype_cheap_isidentifier18<uint8_t> >(data,SIZE, "uint8_t isIdentifier 18");
    test_bool_faked<uint8_t, ctype_cheap_isidentifier20<uint8_t> >(data,SIZE, "uint8_t isIdentifier 20");
    test_bool_faked<uint8_t, ctype_cheap_isidentifier2<uint8_t> >(data,SIZE, "uint8_t isIdentifier 2a");
    test_bool_faked<uint8_t, ctype_cheap_isidentifier4<uint8_t> >(data,SIZE, "uint8_t isIdentifier 4a");
    test_bool_faked<uint8_t, ctype_cheap_isidentifier6<uint8_t> >(data,SIZE, "uint8_t isIdentifier 6a");
    test_bool_faked<uint8_t, ctype_cheap_isidentifier8<uint8_t> >(data,SIZE, "uint8_t isIdentifier 8a");
    test_bool_faked<uint8_t, ctype_cheap_isidentifier10<uint8_t> >(data,SIZE, "uint8_t isIdentifier 10a");
    test_bool_faked<uint8_t, ctype_cheap_isidentifier12<uint8_t> >(data,SIZE, "uint8_t isIdentifier 12a");
    test_bool_faked<uint8_t, ctype_cheap_isidentifier14<uint8_t> >(data,SIZE, "uint8_t isIdentifier 14a");
    test_bool_faked<uint8_t, ctype_cheap_isidentifier16<uint8_t> >(data,SIZE, "uint8_t isIdentifier 16a");
    test_bool_faked<uint8_t, ctype_cheap_isidentifier18<uint8_t> >(data,SIZE, "uint8_t isIdentifier 18a");
    test_bool_faked<uint8_t, ctype_cheap_isidentifier20<uint8_t> >(data,SIZE, "uint8_t isIdentifier 20a");

    summarize("uint8_t ctype complex test", SIZE, iterations, kDontShowGMeans, kDontShowPenalty );


    return 0;
}

// the end
/******************************************************************************/
/******************************************************************************/
