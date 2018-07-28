/*
    Copyright 2010 Adobe Systems Incorporated
    Copyright 2018 Chris Cox
    Distributed under the MIT License (see accompanying file LICENSE_1_0_0.txt
    or a copy at http://stlab.adobe.com/licenses.html )


Goal: Examine performance of common random number generators and corresponding seed setting routines.

Assumptions:
    1) Random number generation will be well optimized.



TODO - template tests take 30-45 minutes!
    Some of the routines are fast, and some are really, really slow

TODO - mac only?
uint32_t arc4random(void);
void arc4random_buf(void *buf, size_t nbytes);
uint32_t arc4random_uniform(uint32_t upper_bound);

*/

#include <cstdio>
#include <cstdlib>
using namespace std;
#include <time.h>
#include <string>
#include <deque>
#include <random>
#include <algorithm>
#include "benchmark_stdint.hpp"
#include "benchmark_timer.h"
#include "benchmark_results.h"

/******************************************************************************/
/******************************************************************************/

// this constant may need to be adjusted to give reasonable minimum times
// For best results, times should be about 1.0 seconds for the minimum test run
int iterations = 200000000;

int32_t init_value = 333;

typedef deque<string> stringPool;

/******************************************************************************/
/******************************************************************************/

volatile static uint32_t gRand32Seed = 0x42424242;

void srand32_local(uint32_t seed)
{
// system implementation may have locks on the global value
// (which should be thread local at worst)
    gRand32Seed = seed;
}

/******************************************************************************/

int32_t rand32_local(void)
{
// system implementation may have locks on the global value
// (which should be thread local at worst)
    uint32_t a = 1103515245;
    uint32_t c = 12345;
    uint32_t temp = (gRand32Seed * a) + c;
    gRand32Seed = temp;
    return int32_t(temp);
}

/******************************************************************************/
/******************************************************************************/

volatile static uint64_t gRand64Seed = 0x42424242;

void srand64_local(uint64_t seed)
{
// system implementation may have locks on the global value
// (which should be thread local at worst)
    gRand64Seed = seed;
}

/******************************************************************************/

int64_t rand64_local(void)
{
// system implementation may have locks on the global value
// (which should be thread local at worst)
    uint64_t a = 6364136223846793005ULL;
    uint64_t c = 1442695040888963407ULL;
    uint64_t temp = (gRand64Seed * a) + c;
    gRand64Seed = temp;
    return int64_t(temp);
}

/******************************************************************************/

// ISO C standard
int32_t randr32_local( uint32_t *seed )
{
    uint32_t result;
    uint32_t theSeed = *seed;
    
    theSeed *= 1103515245;
    theSeed += 12345;
    result = (uint32_t) (theSeed >> 16) & 2047;

    theSeed *= 1103515245;
    theSeed += 12345;
    result ^= (uint32_t) (theSeed >> 16) & 1023;

    theSeed *= 1103515245;
    theSeed += 12345;
    result <<= 10;
    result ^= (uint32_t) (theSeed >> 16) & 1023;

    *seed = theSeed;
    
    return int32_t(result);
}

/******************************************************************************/
/******************************************************************************/


template<class _URNG>
void test_one_generator( _URNG &g, const char *gen_label, int32_t &sum, double &sumDouble, stringPool &stringStorage )
{
    int i;
    
    uniform_int_distribution<int32_t> uniformInts( -9999, 9999);
    uniform_real_distribution<float> uniformFloats(-9999, 9999);
    normal_distribution<float>  normalFloats(100.0, 15.0);
    lognormal_distribution<float>  logNormalFloats(1e-9, 15.0);

    string label0("seed ");
    label0 += gen_label;
    stringStorage.push_back(label0);
    start_timer();
    for (i = 0; i != iterations; ++i)
        {
        g.seed(init_value+i);
        }
    record_result( timer(), stringStorage.back().c_str() );

    string label1("uniform_int_distribution ");
    label1 += gen_label;
    stringStorage.push_back(label1);
    start_timer();
    for (i = 0; i != iterations; ++i)
        {
        sum += uniformInts(g);
        }
    record_result( timer(), stringStorage.back().c_str() );
    
    string label2("uniform_real_distribution ");
    label2 += gen_label;
    stringStorage.push_back(label2);
    start_timer();
    for (i = 0; i != iterations; ++i)
        {
        sumDouble += uniformFloats(g);
        }
    record_result( timer(), stringStorage.back().c_str() );
    
    string label3("generate_canonical<double, 20> ");
    label3 += gen_label;
    stringStorage.push_back(label3);
    start_timer();
    for (i = 0; i != iterations; ++i)
        {
        sumDouble += generate_canonical<double, 20>(g);
        }
    record_result( timer(), stringStorage.back().c_str() );
    
    string label4("normal_distribution ");
    label4 += gen_label;
    stringStorage.push_back(label4);
    start_timer();
    for (i = 0; i != iterations; ++i)
        {
        sumDouble += normalFloats(g);
        }
    record_result( timer(), stringStorage.back().c_str() );
    
    string label5("lognormal_distribution ");
    label5 += gen_label;
    stringStorage.push_back(label5);
    start_timer();
    for (i = 0; i != iterations; ++i)
        {
        sumDouble += logNormalFloats(g);
        }
    record_result( timer(), stringStorage.back().c_str() );

}

/******************************************************************************/
/******************************************************************************/

int main (int argc, char *argv[])
{
    int i;
    int32_t sum = 0;
    double sumDouble = 0;
    
    // output command for documentation:
    for (i = 0; i < argc; ++i)
        printf("%s ", argv[i] );
    printf("\n");

    if (argc > 1) iterations = atoi(argv[1]);
    if (argc > 2) init_value = atoi(argv[2]);


// seed setting
    start_timer();
    for (i = 0; i != iterations; ++i)
        {
        srand((unsigned)(init_value + i));
        }
    record_result( timer(), "srand");
    
    start_timer();
    for (i = 0; i != iterations; ++i)
        {
        srand32_local(init_value + i);
        }
    record_result( timer(), "srand_simple32");
    
    start_timer();
    for (i = 0; i != iterations; ++i)
        {
        srand64_local(init_value + i);
        }
    record_result( timer(), "srand_simple64");

#if !_WIN32
// Windows is missing a lot of standard calls
    start_timer();
    for (i = 0; i != iterations; ++i)
        {
        srandom((unsigned)(init_value + i));
        }
    record_result( timer(), "srandom");
    
    start_timer();
    for (i = 0; i != iterations; ++i)
        {
        srand48(init_value + i);
        }
    record_result( timer(), "srand48");
    
    // call lcong48 first, so seed48 will reset the multiplicand and addend to defaults
    uint16_t seeds16long[7];
    seeds16long[0] = 0x4242;
    seeds16long[2] = 0xABAD;
    seeds16long[3] = 0xBEEF;
    seeds16long[4] = 0xA1C3;
    seeds16long[5] = 0xDEAD;
    seeds16long[6] = 0x5555;
    start_timer();
    for (i = 0; i != iterations; ++i)
        {
        seeds16long[1] = init_value + i;
        lcong48(seeds16long);
        }
    record_result( timer(), "lcong48");
    
    uint16_t seeds16[3];
    seeds16[0] = 0x4242;
    seeds16[2] = 0xBEEF;
    start_timer();
    for (i = 0; i != iterations; ++i)
        {
        seeds16[1] = (uint16_t)(init_value + i);
        seed48(seeds16);
        }
    record_result( timer(), "seed48");
#endif

    summarize("random seeding", 1, iterations, kDontShowGMeans, kDontShowPenalty );

 
 
// random number generation
    
    start_timer();
    for (i = 0; i != iterations; ++i)
        {
        sum += rand();
        }
    record_result( timer(), "rand");
    
    sum = init_value;
    start_timer();
    for (i = 0; i != iterations; ++i)
        {
        sum += rand32_local();
        }
    record_result( timer(), "rand_simple32");
    
    int64_t sum64 = init_value;
    start_timer();
    for (i = 0; i != iterations; ++i)
        {
        sum64 += rand64_local();
        }
    record_result( timer(), "rand_simple64");

    unsigned my_seed = init_value;
    start_timer();
    for (i = 0; i != iterations; ++i)
        {
        sum += randr32_local( &my_seed );
        }
    record_result( timer(), "rand_simple32_r");


    
#if !_WIN32
// Windows is missing a lot of standard calls
    start_timer();
    for (i = 0; i != iterations; ++i)
        {
        sum += rand_r( &my_seed );
        }
    record_result( timer(), "rand_r");

    start_timer();
    for (i = 0; i != iterations; ++i)
        {
        sum += random();
        }
    record_result( timer(), "random");

    start_timer();
    for (i = 0; i != iterations; ++i)
        {
        sum += lrand48();
        }
    record_result( timer(), "lrand48");

    start_timer();
    for (i = 0; i != iterations; ++i)
        {
        sum += nrand48(seeds16);
        }
    record_result( timer(), "nrand48");

    start_timer();
    for (i = 0; i != iterations; ++i)
        {
        sum += mrand48();
        }
    record_result( timer(), "mrand48");

    start_timer();
    for (i = 0; i != iterations; ++i)
        {
        sum += jrand48(seeds16);
        }
    record_result( timer(), "jrand48");
    
    start_timer();
    for (i = 0; i != iterations; ++i)
        {
        sumDouble += drand48();
        }
    record_result( timer(), "drand48");
    
    start_timer();
    for (i = 0; i != iterations; ++i)
        {
        sumDouble += erand48(seeds16);
        }
    record_result( timer(), "erand48");
#endif

    summarize("random values", 1, iterations, kDontShowGMeans, kDontShowPenalty );
    
    

// C++11 templates
    minstd_rand minstd_lc_gen(init_value+42);        // "Minimum Standard" LCG
    knuth_b knuth_shuffle_gen(init_value+42);        // Knuth Shuffle
    ranlux24 ranlux24_gen(init_value+42);            // RANLUX 24 bit
    ranlux48 ranlux48_gen(init_value+42);            // RANLUX 48 bit
    mt19937 twister32_gen(init_value+42);            // Mersenne Twister 32 bit
    mt19937_64 twister64_gen(init_value+42);         // Mersenne Twister 64 bit
 
    stringPool stringStorage;    // so all label strings stay alive until we print the results

    test_one_generator( minstd_lc_gen, "minstd_rand", sum, sumDouble, stringStorage );
    test_one_generator( knuth_shuffle_gen, "knuth_b", sum, sumDouble, stringStorage );
    test_one_generator( ranlux24_gen, "ranlux24", sum, sumDouble, stringStorage );
    test_one_generator( ranlux48_gen, "ranlux48", sum, sumDouble, stringStorage );
    test_one_generator( twister64_gen, "mt19937", sum, sumDouble, stringStorage );
    test_one_generator( twister64_gen, "mt199937_64", sum, sumDouble, stringStorage );

    summarize("std random templates", 1, iterations, kDontShowGMeans, kDontShowPenalty );


    return 0;
}

