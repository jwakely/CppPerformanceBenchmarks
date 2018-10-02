/*
    Copyright 2008 Adobe Systems Incorporated
    Copyright 2018 Chris Cox
    Distributed under the MIT License (see accompanying file LICENSE_1_0_0.txt
    or a copy at http://stlab.adobe.com/licenses.html )


Goal:  Test compiler optimizations related to byte order reversal.


Assumptions:

    1) The compiler will optimize byte order reversal.

    2) The compiler will recognize common idioms for byte order reversal
        and substitute optimal code.

    3) System byte order reversal functions will be fast.


NOTE:  Some processors have special instructions for byte order reversal.
        They generally should be used if available.

*/

#include "benchmark_stdint.hpp"
#include <cstddef>
#include <cstdio>
#include <ctime>
#include <cstdlib>

// TODO - ccox - clean up the macro tests, separate into semi-sane groups
#if defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
#define isBSD   1
#endif

#if (defined(_LINUX_TYPES_H) || defined(_SYS_TYPES_H)) && !defined (__sun)
#include <endian.h>
#endif

#if !defined(_WIN32)
#include <arpa/inet.h>
#endif

#include "benchmark_results.h"
#include "benchmark_timer.h"

/******************************************************************************/

// this constant may need to be adjusted to give reasonable minimum times
// For best results, times should be about 1.0 seconds for the minimum test run
int iterations = 2000000;


// 8000 items, or between 8k and 64k of data
// this is intended to remain within the L2 cache of most common CPUs
const int SIZE = 8000;


// initial value for filling our arrays, may be changed from the command line
uint64_t init_value = 0x1122334455667788;

/******************************************************************************/

// our global arrays of numbers to be operated upon

uint64_t data64unsigned[SIZE];

uint32_t data32unsigned[SIZE];

uint16_t data16unsigned[SIZE];

/******************************************************************************/
/******************************************************************************/

struct swab16 {
    static uint16_t do_shift(uint16_t input) { return ((input >> 8) | (input << 8)); }
};

/******************************************************************************/

struct swab16_mask1 {
    static uint16_t do_shift(uint16_t input) { return (((input & 0xFF00) >> 8) | ((input & 0x00FF) << 8)); }
};

/******************************************************************************/

struct swab16_mask2 {
    static uint16_t do_shift(uint16_t input) { return ( ((input >> 8) & 0x00FF) | ((input << 8) & 0xFF00) ); }
};

/******************************************************************************/

struct swab16_mask3 {
    static uint16_t do_shift(uint16_t input) { return (((input & 0xFF00) >> 8) + ((input & 0x00FF) << 8)); }
};

/******************************************************************************/

struct swab16_mask4 {
    static uint16_t do_shift(uint16_t input) { return ( ((input >> 8) & 0x00FF) + ((input << 8) & 0xFF00) ); }
};

/******************************************************************************/

struct swab16_mask5 {
    static uint16_t do_shift(uint16_t input) { return (((input & 0xFF00) >> 8) ^ ((input & 0x00FF) << 8)); }
};

/******************************************************************************/

struct swab16_mask6 {
    static uint16_t do_shift(uint16_t input) { return ( ((input >> 8) & 0x00FF) ^ ((input << 8) & 0xFF00) ); }
};

/******************************************************************************/

struct swab16_temp1 {
    static uint16_t do_shift(uint16_t input) { 
        uint16_t temp1 = (input >> 8) & 0x00FF;
        uint16_t temp2 = input & 0x00FF;
        return ( temp1 | (temp2 << 8) );
    }
};

/******************************************************************************/

struct swab16_temp2 {
    static uint16_t do_shift(uint16_t input) {
        uint16_t in = input;
        uint16_t out;
        uint8_t *inPtr = (uint8_t *) &in;
        uint8_t *outPtr = (uint8_t *) &out;
        outPtr[0] = inPtr[1];
        outPtr[1] = inPtr[0];
        return out;
    }
};

/******************************************************************************/
/******************************************************************************/

struct swab32 {
    static uint32_t do_shift(uint32_t input) {
        return (  (input >> 24)
                | (input << 24)
                | ((input >> 8) & 0x0000FF00)
                | ((input << 8) & 0x00FF0000));
    }
};

/******************************************************************************/

struct swab32_mask1 {
    static uint32_t do_shift(uint32_t input) {
        return (  ((input & 0xFF000000) >> 24)
                | ((input & 0x000000FF) << 24)
                | ((input & 0x00FF0000) >> 8)
                | ((input & 0x0000FF00) << 8) );
    }
};

/******************************************************************************/

struct swab32_mask2 {
    static uint32_t do_shift(uint32_t input) {
        return (  ((input >> 24) & 0x000000FF)
                | ((input << 24) & 0xFF000000)
                | ((input >> 8) & 0x0000FF00)
                | ((input << 8) & 0x00FF0000) );
    }
};

/******************************************************************************/

struct swab32_mask3 {
    static uint32_t do_shift(uint32_t input) {
        return (  ((input & 0xFF000000) >> 24)
                + ((input & 0x000000FF) << 24)
                + ((input & 0x00FF0000) >> 8)
                + ((input & 0x0000FF00) << 8) );
    }
};

/******************************************************************************/

struct swab32_mask4 {
    static uint32_t do_shift(uint32_t input) {
        return (  ((input >> 24) & 0x000000FF)
                + ((input << 24) & 0xFF000000)
                + ((input >> 8) & 0x0000FF00)
                + ((input << 8) & 0x00FF0000) );
    }
};

/******************************************************************************/

struct swab32_mask5 {
    static uint32_t do_shift(uint32_t input) {
        return (  ((input & 0xFF000000) >> 24)
                ^ ((input & 0x000000FF) << 24)
                ^ ((input & 0x00FF0000) >> 8)
                ^ ((input & 0x0000FF00) << 8) );
    }
};

/******************************************************************************/

struct swab32_mask6 {
    static uint32_t do_shift(uint32_t input) {
        return (  ((input >> 24) & 0x000000FF)
                ^ ((input << 24) & 0xFF000000)
                ^ ((input >> 8) & 0x0000FF00)
                ^ ((input << 8) & 0x00FF0000) );
    }
};

/******************************************************************************/

struct swab32_temp1 {
    static uint32_t do_shift(uint32_t input) {
        uint32_t temp1 = (input >> 24) & 0x000000FF;
        uint32_t temp2 = (input >> 8 ) & 0x0000FF00;
        uint32_t temp3 = (input << 8 ) & 0x00FF0000;
        uint32_t temp4 = (input << 24) & 0xFF000000;
        return ( temp1 | temp2 | temp3 | temp4 );
    }
};

/******************************************************************************/

struct swab32_subset1 {
    static uint32_t do_shift(uint32_t input) {
        uint32_t temp1 = ((uint32_t)swab16::do_shift( (uint16_t)input )) << 16;
        uint32_t temp2 = (uint32_t)swab16::do_shift( (uint16_t)(input >> 16) );
        return ( temp1 | temp2 );
    }
};

/******************************************************************************/

struct swab32_temp2 {
    static uint32_t do_shift(uint32_t input) {
        uint32_t in = input;
        uint32_t out;
        uint8_t *inPtr = (uint8_t *) &in;
        uint8_t *outPtr = (uint8_t *) &out;
        outPtr[0] = inPtr[3];
        outPtr[1] = inPtr[2];
        outPtr[2] = inPtr[1];
        outPtr[3] = inPtr[0];
        return out;
    }
};

/******************************************************************************/

// excessive, but found in an IBM whitepaper
struct swab32_temp3 {
    static uint32_t do_shift(uint32_t input) {
        uint32_t temp1 = (input >> 24) & 0x000000FF;
        uint32_t temp2 = (input >> 16) & 0x000000FF;
        uint32_t temp3 = (input >>  8) & 0x000000FF;
        uint32_t temp4 = input & 0x000000FF;
        return ( temp1 | (temp2 << 8) | (temp3 << 16) | (temp4 << 24) );
    }
};

/******************************************************************************/
/******************************************************************************/

struct swab64 {
    static uint64_t do_shift(uint64_t input) {
        return (  (input >> 56)
                | (input << 56)
                | ((input >> 40) & 0x000000000000FF00ULL)
                | ((input << 40) & 0x00FF000000000000ULL)
                | ((input >> 24) & 0x0000000000FF0000ULL)
                | ((input << 24) & 0x0000FF0000000000ULL)
                | ((input >>  8) & 0x00000000FF000000ULL)
                | ((input <<  8) & 0x000000FF00000000ULL)
                );
    }
};

/******************************************************************************/

struct swab64_mask1 {
    static uint64_t do_shift(uint64_t input) {
        return (  ((input >> 56) & 0x00000000000000FFULL)
                | ((input << 56) & 0xFF00000000000000ULL)
                | ((input >> 40) & 0x000000000000FF00ULL)
                | ((input << 40) & 0x00FF000000000000ULL)
                | ((input >> 24) & 0x0000000000FF0000ULL)
                | ((input << 24) & 0x0000FF0000000000ULL)
                | ((input >>  8) & 0x00000000FF000000ULL)
                | ((input <<  8) & 0x000000FF00000000ULL)
                );
    }
};

/******************************************************************************/

struct swab64_mask2 {
    static uint64_t do_shift(uint64_t input) {
        return (  ((input & 0xFF00000000000000ULL) >> 56)
                | ((input & 0x00FF000000000000ULL) >> 40)
                | ((input & 0x0000FF0000000000ULL) >> 24)
                | ((input & 0x000000FF00000000ULL) >>  8)
                | ((input & 0x00000000000000FFULL) << 56)
                | ((input & 0x000000000000FF00ULL) << 40)
                | ((input & 0x0000000000FF0000ULL) << 24)
                | ((input & 0x00000000FF000000ULL) <<  8)
                );
    }
};

/******************************************************************************/

struct swab64_mask3 {
    static uint64_t do_shift(uint64_t input) {
        return (  ((input >> 56) & 0x00000000000000FFULL)
                + ((input << 56) & 0xFF00000000000000ULL)
                + ((input >> 40) & 0x000000000000FF00ULL)
                + ((input << 40) & 0x00FF000000000000ULL)
                + ((input >> 24) & 0x0000000000FF0000ULL)
                + ((input << 24) & 0x0000FF0000000000ULL)
                + ((input >>  8) & 0x00000000FF000000ULL)
                + ((input <<  8) & 0x000000FF00000000ULL)
                );
    }
};

/******************************************************************************/

// this appears to be hit a codegen bug on MSVC 14.12
struct swab64_mask4 {
    static uint64_t do_shift(uint64_t input) {
        return (  ((input & 0xFF00000000000000ULL) >> 56)
                + ((input & 0x00FF000000000000ULL) >> 40)
                + ((input & 0x0000FF0000000000ULL) >> 24)
                + ((input & 0x000000FF00000000ULL) >>  8)
                + ((input & 0x00000000000000FFULL) << 56)
                + ((input & 0x000000000000FF00ULL) << 40)
                + ((input & 0x0000000000FF0000ULL) << 24)
                + ((input & 0x00000000FF000000ULL) <<  8)
                );
    }
};

/******************************************************************************/

struct swab64_mask5 {
    static uint64_t do_shift(uint64_t input) {
        return (  ((input >> 56) & 0x00000000000000FFULL)
                ^ ((input >> 40) & 0x000000000000FF00ULL)
                ^ ((input >> 24) & 0x0000000000FF0000ULL)
                ^ ((input >>  8) & 0x00000000FF000000ULL)
                ^ ((input << 56) & 0xFF00000000000000ULL)
                ^ ((input << 40) & 0x00FF000000000000ULL)
                ^ ((input << 24) & 0x0000FF0000000000ULL)
                ^ ((input <<  8) & 0x000000FF00000000ULL)
                );
    }
};

/******************************************************************************/

struct swab64_mask6 {
    static uint64_t do_shift(uint64_t input) {
        return (  ((input & 0xFF00000000000000ULL) >> 56)
                ^ ((input & 0x00FF000000000000ULL) >> 40)
                ^ ((input & 0x0000FF0000000000ULL) >> 24)
                ^ ((input & 0x000000FF00000000ULL) >>  8)
                ^ ((input & 0x00000000000000FFULL) << 56)
                ^ ((input & 0x000000000000FF00ULL) << 40)
                ^ ((input & 0x0000000000FF0000ULL) << 24)
                ^ ((input & 0x00000000FF000000ULL) <<  8)
                );
    }
};

/******************************************************************************/

struct swab64_temp1 {
    static uint64_t do_shift(uint64_t input) {
        uint64_t temp1 = (input >> 56) & 0x00000000000000FFULL;
        uint64_t temp3 = (input >> 40) & 0x000000000000FF00ULL;
        uint64_t temp5 = (input >> 24) & 0x0000000000FF0000ULL;
        uint64_t temp7 = (input >>  8) & 0x00000000FF000000ULL;
        uint64_t temp2 = (input << 56) & 0xFF00000000000000ULL;
        uint64_t temp4 = (input << 40) & 0x00FF000000000000ULL;
        uint64_t temp6 = (input << 24) & 0x0000FF0000000000ULL;
        uint64_t temp8 = (input <<  8) & 0x000000FF00000000ULL;
        return ( temp1 | temp2 | temp3 | temp4 | temp5 | temp6 | temp7 | temp8 );
    }
};

/******************************************************************************/

struct swab64_subset1 {
    static uint64_t do_shift(uint64_t input) {
        uint64_t temp1 = ((uint64_t)swab32::do_shift( (uint32_t)input )) << 32;
        uint64_t temp2 = (uint64_t)swab32::do_shift( (uint32_t)(input >> 32) );
        return ( temp1 | temp2 );
    }
};

/******************************************************************************/

struct swab64_temp2 {
    static uint64_t do_shift(uint64_t input) {
        uint64_t in = input;
        uint64_t out;
        uint8_t *inPtr = (uint8_t *) &in;
        uint8_t *outPtr = (uint8_t *) &out;
        outPtr[0] = inPtr[7];
        outPtr[1] = inPtr[6];
        outPtr[2] = inPtr[5];
        outPtr[3] = inPtr[4];
        outPtr[4] = inPtr[3];
        outPtr[5] = inPtr[2];
        outPtr[6] = inPtr[1];
        outPtr[7] = inPtr[0];
        return out;
    }
};

/******************************************************************************/

struct swab64_temp3 {
    static uint64_t do_shift(uint64_t input) {
        uint64_t temp1 = (input >> 56) & 0x00000000000000FFULL;
        uint64_t temp2 = (input >> 48) & 0x00000000000000FFULL;
        uint64_t temp3 = (input >> 40) & 0x00000000000000FFULL;
        uint64_t temp4 = (input >> 32) & 0x00000000000000FFULL;
        uint64_t temp5 = (input >> 24) & 0x00000000000000FFULL;
        uint64_t temp6 = (input >> 16) & 0x00000000000000FFULL;
        uint64_t temp7 = (input >>  8) & 0x00000000000000FFULL;
        uint64_t temp8 = input         & 0x00000000000000FFULL;
        return ( temp1 | (temp2 << 8) | (temp3 << 16) | (temp4 << 24) 
                | (temp5 << 32) | (temp6 << 40) | (temp7 << 48) | (temp8 << 56) );
    }
};

/******************************************************************************/

#if !defined(_WIN32)
struct swab_htonl {
    static uint16_t do_shift(uint16_t input) {
        return htons(input);
    }
    static uint32_t do_shift(uint32_t input) {
        return htonl(input);
    }

#if !(defined(_LINUX_TYPES_H) || defined(_SYS_TYPES_H) || defined(isBSD))
    static uint64_t do_shift(uint64_t input) {
        return htonll(input);
    }
#endif

};
#endif

/******************************************************************************/

#if !defined(_WIN32)
struct swab_ntohl {
    static uint16_t do_shift(uint16_t input) {
        return ntohs(input);
    }
    static uint32_t do_shift(uint32_t input) {
        return ntohl(input);
    }

#if !(defined(_LINUX_TYPES_H) || defined(_SYS_TYPES_H) || defined(isBSD))
    static uint64_t do_shift(uint64_t input) {
        return ntohll(input);
    }
#endif

};
#endif

/******************************************************************************/

#if (defined(_LINUX_TYPES_H) || defined(_SYS_TYPES_H)) && !defined (__sun)
struct swab_htobe {
    static uint16_t do_shift(uint16_t input) {
        return htobe16(input);
    }
    static uint32_t do_shift(uint32_t input) {
        return htobe32(input);
    }
    static uint64_t do_shift(uint64_t input) {
        return htobe64(input);
    }
};
#endif

/******************************************************************************/

#if (defined(_LINUX_TYPES_H) || defined(_SYS_TYPES_H)) && !defined (__sun)
struct swab_htole {
    static uint16_t do_shift(uint16_t input) {
        return htole16(input);
    }
    static uint32_t do_shift(uint32_t input) {
        return htole32(input);
    }
    static uint64_t do_shift(uint64_t input) {
        return htole64(input);
    }
};
#endif

/******************************************************************************/
/******************************************************************************/

#include "benchmark_shared_tests.h"

/******************************************************************************/
/******************************************************************************/

int main(int argc, char** argv) {
  
    // output command for documentation:
    int i;
    for (i = 0; i < argc; ++i)
        printf("%s ", argv[i] );
    printf("\n");

    if (argc > 1) iterations = atoi(argv[1]);
    if (argc > 2) init_value = (long) atoi(argv[2]);




// verify correct results        (grumble, grumble compiler bugs)
    const uint16_t testPattern16 = 0x0102;
    const uint16_t reversedPattern16 = 0x0201;
    const uint32_t testPattern32 = 0x01020304UL;
    const uint32_t reversedPattern32 = 0x04030201UL;
    const uint64_t testPattern64 = 0x0102030405060708ULL;
    const uint64_t reversedPattern64 = 0x0807060504030201ULL;


    if ( reversedPattern16 != swab16::do_shift(testPattern16) )
        printf("swab16 got incorrect results\n");
    if ( reversedPattern16 != swab16_mask1::do_shift(testPattern16) )
        printf("swab16_mask1 got incorrect results\n");
    if ( reversedPattern16 != swab16_mask2::do_shift(testPattern16) )
        printf("swab16_mask2 got incorrect results\n");
    if ( reversedPattern16 != swab16_mask3::do_shift(testPattern16) )
        printf("swab16_mask3 got incorrect results\n");
    if ( reversedPattern16 != swab16_mask4::do_shift(testPattern16) )
        printf("swab16_mask4 got incorrect results\n");
    if ( reversedPattern16 != swab16_mask5::do_shift(testPattern16) )
        printf("swab16_mask5 got incorrect results\n");
    if ( reversedPattern16 != swab16_mask6::do_shift(testPattern16) )
        printf("swab16_mask6 got incorrect results\n");
    if ( reversedPattern16 != swab16_temp1::do_shift(testPattern16) )
        printf("swab16_temp1 got incorrect results\n");
    if ( reversedPattern16 != swab16_temp2::do_shift(testPattern16) )
        printf("swab16_temp2 got incorrect results\n");

    if ( reversedPattern32 != swab32::do_shift(testPattern32) )
        printf("swab32 got incorrect results\n");
    if ( reversedPattern32 != swab32_mask1::do_shift(testPattern32) )
        printf("swab32_mask1 got incorrect results\n");
    if ( reversedPattern32 != swab32_mask2::do_shift(testPattern32) )
        printf("swab32_mask2 got incorrect results\n");
    if ( reversedPattern32 != swab32_mask3::do_shift(testPattern32) )
        printf("swab32_mask3 got incorrect results\n");
    if ( reversedPattern32 != swab32_mask4::do_shift(testPattern32) )
        printf("swab32_mask4 got incorrect results\n");
    if ( reversedPattern32 != swab32_mask5::do_shift(testPattern32) )
        printf("swab32_mask5 got incorrect results\n");
    if ( reversedPattern32 != swab32_mask6::do_shift(testPattern32) )
        printf("swab32_mask6 got incorrect results\n");
    if ( reversedPattern32 != swab32_temp1::do_shift(testPattern32) )
        printf("swab32_temp1 got incorrect results\n");
    if ( reversedPattern32 != swab32_temp2::do_shift(testPattern32) )
        printf("swab32_temp2 got incorrect results\n");
    if ( reversedPattern32 != swab32_subset1::do_shift(testPattern32) )
        printf("swab32_subset1 got incorrect results\n");

    if ( reversedPattern64 != swab64::do_shift(testPattern64) )
        printf("swab64 got incorrect results\n");
    if ( reversedPattern64 != swab64_mask1::do_shift(testPattern64) )
        printf("swab64_mask1 got incorrect results\n");
    if ( reversedPattern64 != swab64_mask2::do_shift(testPattern64) )
        printf("swab64_mask2 got incorrect results\n");
    if ( reversedPattern64 != swab64_mask3::do_shift(testPattern64) )
        printf("swab64_mask3 got incorrect results\n");
    if ( reversedPattern64 != swab64_mask4::do_shift(testPattern64) )
        printf("swab64_mask4 got incorrect results\n");
    if ( reversedPattern64 != swab64_mask5::do_shift(testPattern64) )
        printf("swab64_mask5 got incorrect results\n");
    if ( reversedPattern64 != swab64_mask6::do_shift(testPattern64) )
        printf("swab64_mask6 got incorrect results\n");
    if ( reversedPattern64 != swab64_temp1::do_shift(testPattern64) )
        printf("swab64_temp1 got incorrect results\n");
    if ( reversedPattern64 != swab64_temp2::do_shift(testPattern64) )
        printf("swab64_temp2 got incorrect results\n");
    if ( reversedPattern64 != swab64_subset1::do_shift(testPattern64) )
        printf("swab64_subset1 got incorrect results\n");




// now test performance
    fill(data16unsigned, data16unsigned+SIZE, uint16_t(init_value));
    fill(data32unsigned, data32unsigned+SIZE, uint32_t(init_value));
    fill(data64unsigned, data64unsigned+SIZE, uint64_t(init_value));


    test_constant<uint16_t, swab16 >(data16unsigned, SIZE, "uint16_t byte order reverse1");
    test_constant<uint16_t, swab16_mask1 >(data16unsigned, SIZE, "uint16_t byte order reverse2");
    test_constant<uint16_t, swab16_mask2 >(data16unsigned, SIZE, "uint16_t byte order reverse3");
    test_constant<uint16_t, swab16_mask3 >(data16unsigned, SIZE, "uint16_t byte order reverse4");
    test_constant<uint16_t, swab16_mask4 >(data16unsigned, SIZE, "uint16_t byte order reverse5");
    test_constant<uint16_t, swab16_mask5 >(data16unsigned, SIZE, "uint16_t byte order reverse6");
    test_constant<uint16_t, swab16_mask6 >(data16unsigned, SIZE, "uint16_t byte order reverse7");
    test_constant<uint16_t, swab16_temp1 >(data16unsigned, SIZE, "uint16_t byte order reverse8");
    test_constant<uint16_t, swab16_temp2 >(data16unsigned, SIZE, "uint16_t byte order reverse9");
    
    summarize("Byte Order Reverse 16bit", SIZE, iterations, kDontShowGMeans, kDontShowPenalty );
    
    
    test_constant<uint32_t, swab32 >(data32unsigned, SIZE, "uint32_t byte order reverse1");
    test_constant<uint32_t, swab32_mask1 >(data32unsigned, SIZE, "uint32_t byte order reverse2");
    test_constant<uint32_t, swab32_mask2 >(data32unsigned, SIZE, "uint32_t byte order reverse3");
    test_constant<uint32_t, swab32_mask3 >(data32unsigned, SIZE, "uint32_t byte order reverse4");
    test_constant<uint32_t, swab32_mask4 >(data32unsigned, SIZE, "uint32_t byte order reverse5");
    test_constant<uint32_t, swab32_mask5 >(data32unsigned, SIZE, "uint32_t byte order reverse6");
    test_constant<uint32_t, swab32_mask6 >(data32unsigned, SIZE, "uint32_t byte order reverse7");
    test_constant<uint32_t, swab32_temp1 >(data32unsigned, SIZE, "uint32_t byte order reverse8");
    test_constant<uint64_t, swab32_subset1 >(data64unsigned, SIZE, "uint32_t byte order reverse9");
    test_constant<uint64_t, swab32_temp2 >(data64unsigned, SIZE, "uint32_t byte order reverse10");
    test_constant<uint64_t, swab32_temp3 >(data64unsigned, SIZE, "uint32_t byte order reverse11");
    
    summarize("Byte Order Reverse 32bit", SIZE, iterations, kDontShowGMeans, kDontShowPenalty );
    

    test_constant<uint64_t, swab64 >(data64unsigned, SIZE, "uint64_t byte order reverse1");
    test_constant<uint64_t, swab64_mask1 >(data64unsigned, SIZE, "uint64_t byte order reverse2");
    test_constant<uint64_t, swab64_mask2 >(data64unsigned, SIZE, "uint64_t byte order reverse3");
    test_constant<uint64_t, swab64_mask3 >(data64unsigned, SIZE, "uint64_t byte order reverse4");
    test_constant<uint64_t, swab64_mask4 >(data64unsigned, SIZE, "uint64_t byte order reverse5");
    test_constant<uint64_t, swab64_mask5 >(data64unsigned, SIZE, "uint64_t byte order reverse6");
    test_constant<uint64_t, swab64_mask6 >(data64unsigned, SIZE, "uint64_t byte order reverse7");
    test_constant<uint64_t, swab64_temp1 >(data64unsigned, SIZE, "uint64_t byte order reverse8");
    test_constant<uint64_t, swab64_subset1 >(data64unsigned, SIZE, "uint64_t byte order reverse9");
    test_constant<uint64_t, swab64_temp2 >(data64unsigned, SIZE, "uint64_t byte order reverse10");
    test_constant<uint64_t, swab64_temp3 >(data64unsigned, SIZE, "uint64_t byte order reverse11");
    
    summarize("Byte Order Reverse 64bit", SIZE, iterations, kDontShowGMeans, kDontShowPenalty );


    // library/standard functions that do byte order swapping
#if !defined(_WIN32)
    test_constant<uint16_t, swab_htonl >(data16unsigned, SIZE, "uint16_t htons");
    test_constant<uint16_t, swab_ntohl >(data16unsigned, SIZE, "uint16_t ntohs");
    test_constant<uint32_t, swab_htonl >(data32unsigned, SIZE, "uint32_t htonl");
    test_constant<uint32_t, swab_ntohl >(data32unsigned, SIZE, "uint32_t ntohl");

#if !(defined(_LINUX_TYPES_H) || defined(_SYS_TYPES_H) || defined(isBSD))
    test_constant<uint64_t, swab_htonl >(data64unsigned, SIZE, "uint64_t htonll");
    test_constant<uint64_t, swab_ntohl >(data64unsigned, SIZE, "uint64_t ntohll");
#endif

#if (defined(_LINUX_TYPES_H) || defined(_SYS_TYPES_H)) && !defined (__sun) && !defined(isBSD)
    test_constant<uint16_t, swab_htobe >(data16unsigned, SIZE, "uint16_t htobe16");
    test_constant<uint16_t, swab_htole >(data16unsigned, SIZE, "uint16_t htole16");
    test_constant<uint32_t, swab_htobe >(data32unsigned, SIZE, "uint32_t htobe32");
    test_constant<uint32_t, swab_htole >(data32unsigned, SIZE, "uint32_t htole32");
    test_constant<uint64_t, swab_htobe >(data64unsigned, SIZE, "uint64_t htobe64");
    test_constant<uint64_t, swab_htole >(data64unsigned, SIZE, "uint64_t htole64");
#endif

    summarize("Byte Order library functions", SIZE, iterations, kDontShowGMeans, kDontShowPenalty );
#endif


    return 0;
}

// the end
/******************************************************************************/
/******************************************************************************/
