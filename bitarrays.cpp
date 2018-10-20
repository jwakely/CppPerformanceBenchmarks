/*
    Copyright 2008 Adobe Systems Incorporated
    Copyright 2018 Chris Cox
    Distributed under the MIT License (see accompanying file LICENSE_1_0_0.txt
    or a copy at http://stlab.adobe.com/licenses.html )


Goal:  Test compiler optimizations related to manipulation of bitarrays.
        aka: bitvector, bitset, or bitfield


Assumptions:

    1) The compiler will recognize and optimize bit array operations.
    
    2) Proper optimization of bitarrays requires good loop unrolling and algebraic simplification,
        and can include many other optimizations.
    
    3) Recognizing bit operations and aligning to word boundaries provides the greatest speedup.
    
    4) Recognition of byte/word setting loops and converting to memset calls will help some operations.
 
    5) std::bitset operations should be well optimized.


NOTE: This sort of bitarray pattern occurs in std::vector<bool>, std::bitset,
        some memory management systems, raster imaging systems, printers,
        simulation systems, and other odd places.


TODO - use pseudo-random values for bit counting tests.
TODO - unify the tests for different word sizes

*/

/******************************************************************************/

#include "benchmark_stdint.hpp"
#include <stddef.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <math.h>
#include "benchmark_results.h"
#include "benchmark_timer.h"
#include <bitset>

/******************************************************************************/
/******************************************************************************/

// this constant may need to be adjusted to give reasonable minimum times
// For best results, times should be about 1.0 seconds for the minimum test run
// on 3Ghz desktop CPUs, 100k iterations is about 1.0 seconds
int iterations = 800000;


// 16000 bits, or about 2k of data should be safely inside L1 cache
const size_t BITSIZE = 16000;


// fake for shared haeders
const size_t SIZE = 2;


int init_value = 39;    // 0x00100111 = 4 bits


typedef std::bitset<BITSIZE-6> testBitset;

/******************************************************************************/

#include "benchmark_shared_tests.h"

/******************************************************************************/

// our global arrays of numbers to be operated upon

uint64_t data64unsigned[(BITSIZE+63)/(8*sizeof(uint64_t))];
uint64_t data64unsigned2[(BITSIZE+63)/(8*sizeof(uint64_t))];
uint64_t data64unsigned3[(BITSIZE+63)/(8*sizeof(uint64_t))];

uint32_t data32unsigned[(BITSIZE+31)/(8*sizeof(uint32_t))];
uint32_t data32unsigned2[(BITSIZE+31)/(8*sizeof(uint32_t))];
uint32_t data32unsigned3[(BITSIZE+31)/(8*sizeof(uint32_t))];

uint16_t data16unsigned[(BITSIZE+15)/(8*sizeof(uint16_t))];
uint16_t data16unsigned2[(BITSIZE+15)/(8*sizeof(uint16_t))];
uint16_t data16unsigned3[(BITSIZE+15)/(8*sizeof(uint16_t))];

uint8_t data8unsigned[(BITSIZE+7)/(8*sizeof(uint8_t))];
uint8_t data8unsigned2[(BITSIZE+7)/(8*sizeof(uint8_t))];
uint8_t data8unsigned3[(BITSIZE+7)/(8*sizeof(uint8_t))];

/******************************************************************************/

/*
    1 byte   -> x/8   -> x >> 3
    2 bytes  -> x/16  -> x >> 4
    4 bytes  -> x/32  -> x >> 5
    8 bytes  -> x/64  -> x >> 6
    16 bytes -> x/128 -> x >> 7
    
    1 byte   -> x%8   -> x & ((1 << 3)-1)
    2 bytes  -> x%16  -> x & ((1 << 4)-1)
    4 bytes  -> x%32  -> x & ((1 << 5)-1)
    8 bytes  -> x%64  -> x & ((1 << 6)-1)
    16 bytes -> x%128 -> x & ((1 << 7)-1)
*/
const int shiftSizeByBytes[] = { 0, 3, 4, 0, 5, 0, 0, 0, 6, 0, 0, 0, 0, 0, 0, 0, 7 };

/******************************************************************************/
/******************************************************************************/

template<typename T>
void SetBits ( T *table, const size_t start, const size_t stop )
{
    size_t count = stop - start;
    size_t pos = start;
    
    if (stop < start)
        return;
    
    // divide and mod will be by powers of 2:  they optimize to shift and mask
    while (count--) {
        table[ pos / (8*sizeof(T)) ] |= (T(1) << (pos % (8*sizeof(T))));
        pos++;
    }

}

/******************************************************************************/

template<typename T>
void ClearBits ( T *table, const size_t start, const size_t stop )
{
    size_t count = stop - start;
    size_t pos = start;
    
    if (stop < start)
        return;
    
    // divide and mod will be by powers of 2:  optimize to shift and mask
    while (count--) {
        table[ pos / (8*sizeof(T)) ] &= ~(T(1) << (pos % (8*sizeof(T))));
        pos++;
    }

}

/******************************************************************************/

template<typename T>
void InvertBits ( T *table, const size_t start, const size_t stop )
{
    size_t count = stop - start;
    size_t pos = start;
    
    if (stop < start)
        return;
    
    // divide and mod will be by powers of 2:  optimize to shift and mask
    while (count--) {
        table[ pos / (8*sizeof(T)) ] ^= (T(1) << (pos % (8*sizeof(T))));
        pos++;
    }

}

/******************************************************************************/

template<typename T>
void SetBitsShift ( T *table, const size_t start, const size_t stop )
{
    size_t count = stop - start;
    size_t pos = start;
    const int shift = shiftSizeByBytes[ sizeof(T) ];
    const T modMask = (1 << shift) - 1;
    
    if (stop < start)
        return;
    
    // divide and mod replaced with shift and mask
    while (count--) {
        size_t index = pos >> shift;
        T bitMask = T(T(1) << (pos & modMask));
        table[ index ] |= bitMask;
        pos++;
    }

}

/******************************************************************************/

template<typename T>
void ClearBitsShift ( T *table, const size_t start, const size_t stop )
{
    size_t count = stop - start;
    size_t pos = start;
    const int shift = shiftSizeByBytes[ sizeof(T) ];
    const T modMask = (1 << shift) - 1;
    
    if (stop < start)
        return;
    
    // divide and mod replaced with shift and mask
    while (count--) {
        size_t index = pos >> shift;
        T bitMask = T(T(1) << (pos & modMask));
        table[ index ] &= ~bitMask;
        pos++;
    }

}

/******************************************************************************/

template<typename T>
void InvertBitsShift ( T *table, const size_t start, const size_t stop )
{
    size_t count = stop - start;
    size_t pos = start;
    const int shift = shiftSizeByBytes[ sizeof(T) ];
    const T modMask = (1 << shift) - 1;
    
    if (stop < start)
        return;
    
    // divide and mod replaced with shift and mask
    while (count--) {
        size_t index = pos >> shift;
        T bitMask = T(T(1) << (pos & modMask));
        table[ index ] ^= bitMask;
        pos++;
    }

}

/******************************************************************************/

// the result of just recognizing whole words
template<typename T>
void SetBitsHalfOpt ( T *table, const size_t start, const size_t stop )
{
    size_t count = stop - start;
    size_t pos = start;
    
    if (stop < start)
        return;
    
    // align to word boundary
    while (count && (pos % (8*sizeof(T)) != 0)) {
        count--;
        table[ pos / (8*sizeof(T)) ] |= (T(1) << (pos % (8*sizeof(T))));
        pos++;
    }
    
    // whole words
    size_t index = pos / (8*sizeof(T));
    while ( count >= (8*sizeof(T)) ) {
        pos += (8*sizeof(T));
        count -= (8*sizeof(T));
        table[ index++ ] = ~T(0);
    }
    
    // remaining partial word
    while (count--) {
        table[ pos / (8*sizeof(T)) ] |= (T(1) << (pos % (8*sizeof(T))));
        pos++;
    }

}

/******************************************************************************/

// the result of just recognizing whole words
template<typename T>
void ClearBitsHalfOpt ( T *table, const size_t start, const size_t stop )
{
    size_t count = stop - start;
    size_t pos = start;
    
    if (stop < start)
        return;
    
    // align to word boundary
    while (count && (pos % (8*sizeof(T)) != 0)) {
        count--;
        table[ pos / (8*sizeof(T)) ] &= ~(T(1) << (pos % (8*sizeof(T))));
        pos++;
    }
    
    // whole words
    size_t index = pos / (8*sizeof(T));
    while ( count > (8*sizeof(T)) ) {
        pos += (8*sizeof(T));
        count -= (8*sizeof(T));
        table[ index++ ] = T(0);
    }
    
    // remaining partial word
    while (count--) {
        table[ pos / (8*sizeof(T)) ] &= ~(T(1) << (pos % (8*sizeof(T))));
        pos++;
    }

}

/******************************************************************************/

// the result of just recognizing whole words
template<typename T>
void InvertBitsHalfOpt ( T *table, const size_t start, const size_t stop )
{
    size_t count = stop - start;
    size_t pos = start;
    
    if (stop < start)
        return;
    
    // align to word boundary
    while (count && (pos % (8*sizeof(T)) != 0)) {
        count--;
        table[ pos / (8*sizeof(T)) ] ^= (T(1) << (pos % (8*sizeof(T))));
        pos++;
    }
    
    // whole words
    size_t index = pos / (8*sizeof(T));
    while ( count > (8*sizeof(T)) ) {
        pos += (8*sizeof(T));
        count -= (8*sizeof(T));
        table[ index++ ] ^= ~T(0);
    }
    
    // remaining partial word
    while (count--) {
        table[ pos / (8*sizeof(T)) ] ^= (T(1) << (pos % (8*sizeof(T))));
        pos++;
    }

}

/******************************************************************************/

// optimizing the align and cleanup loops into single word operations
// use memset for word fill if the count is large enough to overcome overhead
//        that works for set and clear, but not for invert
template<typename T>
void SetBitsOptimized ( T *table, const size_t start, const size_t stop )
{
    size_t count = stop - start;
    size_t pos = start;
    const int shift = shiftSizeByBytes[ sizeof(T) ];
    const T modMask = (T(1) << shift) - 1;
    
    if (stop < start)
        return;
    
    // first partial word, if we will cross into another word, and if it is not already aligned
    if ( ((stop >> shift) > (start >> shift)) && ((start & modMask) != 0) ) {
        size_t bitstart = start & modMask;
        table[ start >> shift ] |= (~T(0) << bitstart);
        size_t adjust = ((8*sizeof(T)) - bitstart);
        count -= adjust;
        pos += adjust;
    }
    
    // fill all the complete words
    
    // for best performance, the crossover point should be measured (because memset will have some overhead)
    const size_t HUGE_BITRUN = 512;
    
    if ( count > HUGE_BITRUN ) {
        size_t byte_count = sizeof(T) * (count>>shift);
        memset( &table[ pos >> shift ], 255, byte_count );
        pos += 8*byte_count;
        count -= 8*byte_count;
    } else {
        size_t index = pos >> shift;
        while ( count >= (8*sizeof(T)) ) {
            pos += (8*sizeof(T));
            count -= (8*sizeof(T));
            table[ index++ ] = ~T(0);
        }
    }
    
    // last partial word, if necessary
    if (count > 0) {
        T mask1 = T(~T(0) << (pos & modMask));    // pos could be a multiple of 32 or equal to start!
        T mask2 = T(~T(0) << (stop & modMask));
        table[ pos >> shift ] |= mask1 & ~mask2;
    }
}

/******************************************************************************/
/******************************************************************************/

template<typename T>
void AndBitTables ( T *table1, const T *table2, const size_t start, const size_t stop )
{
    size_t count = stop - start;
    size_t pos = start;
    
    if (stop < start)
        return;
    
    // divide and mod will be by powers of 2:  they should always optimize to shift and mask
    while (count--) {
        T word1 = table1[ pos / (8*sizeof(T)) ];
        T bit2 = table2[ pos / (8*sizeof(T)) ] & T(T(1) << (pos % (8*sizeof(T))));
        // clear bit in base, copy bit over from other set, and to get result with other bits still in place
        T result = ((word1 & ~(T(1) << (pos % (8*sizeof(T))))) | bit2) & word1;
        table1[ pos / (8*sizeof(T)) ] = result;
        pos++;
    }

}

/******************************************************************************/

template<typename T>
void OrBitTables ( T *table1, const T *table2, const size_t start, const size_t stop )
{
    size_t count = stop - start;
    size_t pos = start;
    
    if (stop < start)
        return;
    
    // divide and mod will be by powers of 2:  they should always optimize to shift and mask
    while (count--) {
        T word1 = table1[ pos / (8*sizeof(T)) ];
        T bit2 = table2[ pos / (8*sizeof(T)) ] & T(T(1) << (pos % (8*sizeof(T))));
        // or with zeros will leave all other bits in place, only alter desired bit
        T result = (word1 | bit2);
        table1[ pos / (8*sizeof(T)) ] = result;
        pos++;
    }

}

/******************************************************************************/

template<typename T>
void XorBitTables ( T *table1, const T *table2, const size_t start, const size_t stop )
{
    size_t count = stop - start;
    size_t pos = start;
    
    if (stop < start)
        return;
    
    // divide and mod will be by powers of 2:  they should always optimize to shift and mask
    while (count--) {
        T word1 = table1[ pos / (8*sizeof(T)) ];
        T bit2 = table2[ pos / (8*sizeof(T)) ] & T(T(1) << (pos % (8*sizeof(T))));
        // xor with zeros will leave all other bits in place, only alter desired bit
        T result = (word1 ^ bit2);
        table1[ pos / (8*sizeof(T)) ] = result;
        pos++;
    }

}

/******************************************************************************/

template<typename T>
void AndComplimentBitTables ( T *table1, const T *table2, const size_t start, const size_t stop )
{
    size_t count = stop - start;
    size_t pos = start;
    
    if (stop < start)
        return;
    
    // divide and mod will be by powers of 2:  they should always optimize to shift and mask
    while (count--) {
        T word1 = table1[ pos / (8*sizeof(T)) ];
        T mask1 = T(T(1) << (pos % (8*sizeof(T))));
        T bit2 = table2[ pos / (8*sizeof(T)) ] & mask1;
        bit2 ^= mask1;    // invert the single bit
        // clear bit in base, copy bit over from other set, and to get result with other bits still in place
        T result = ((word1 & ~mask1) | bit2) & word1;
        table1[ pos / (8*sizeof(T)) ] = result;
        pos++;
    }

}

/******************************************************************************/
/******************************************************************************/

template<typename T>
void AndBitTablesShift ( T *table1, const T *table2, const size_t start, const size_t stop )
{
    size_t count = stop - start;
    size_t pos = start;
    const int shift = shiftSizeByBytes[ sizeof(T) ];
    const T modMask = (1 << shift) - 1;
    
    if (stop < start)
        return;
    
    // divide and mod replaced with shift and mask
    while (count--) {
        size_t index = pos >> shift;
        T bitMask = T(T(1) << (pos & modMask));
        T word1 = table1 [index ];
        T bit2 = table2[ index] & bitMask;
        // clear bit in base, copy bit over from other set, and to get result with other bits still in place
        T result = ((word1 & ~bitMask) | bit2) & word1;
        table1[ index ] = result;
        pos++;
    }

}

/******************************************************************************/

template<typename T>
void OrBitTablesShift ( T *table1, const T *table2, const size_t start, const size_t stop )
{
    size_t count = stop - start;
    size_t pos = start;
    const int shift = shiftSizeByBytes[ sizeof(T) ];
    const T modMask = (1 << shift) - 1;
    
    if (stop < start)
        return;
    
    // divide and mod replaced with shift and mask
    while (count--) {
        size_t index = pos >> shift;
        T bitMask = T(T(1) << (pos & modMask));
        T word1 = table1 [index ];
        T bit2 = table2[ index] & bitMask;
        // or with zeros will leave all other bits in place, only alter desired bit
        T result = (word1 | bit2);
        table1[ index ] = result;
        pos++;
    }

}

/******************************************************************************/

template<typename T>
void XorBitTablesShift ( T *table1, const T *table2, const size_t start, const size_t stop )
{
    size_t count = stop - start;
    size_t pos = start;
    const int shift = shiftSizeByBytes[ sizeof(T) ];
    const T modMask = (1 << shift) - 1;
    
    if (stop < start)
        return;
    
    // divide and mod replaced with shift and mask
    while (count--) {
        size_t index = pos >> shift;
        T bitMask = T(T(1) << (pos & modMask));
        T word1 = table1 [index ];
        T bit2 = table2[ index] & bitMask;
        // xor with zeros will leave all other bits in place, only alter desired bit
        T result = (word1 ^ bit2);
        table1[ index ] = result;
        pos++;
    }

}

/******************************************************************************/

template<typename T>
void AndComplimentBitTablesShift ( T *table1, const T *table2, const size_t start, const size_t stop )
{
    size_t count = stop - start;
    size_t pos = start;
    const int shift = shiftSizeByBytes[ sizeof(T) ];
    const T modMask = (1 << shift) - 1;
    
    if (stop < start)
        return;
    
    // divide and mod replaced with shift and mask
    while (count--) {
        size_t index = pos >> shift;
        T bitMask = T(T(1) << (pos & modMask));
        T word1 = table1 [index ];
        T bit2 = table2[ index] & bitMask;
        bit2 ^= bitMask;    // invert the single bit
        // clear bit in base, copy bit over from other set, and to get result with other bits still in place
        T result = ((word1 & ~bitMask) | bit2) & word1;
        table1[ index ] = result;
        pos++;
    }

}

/******************************************************************************/
/******************************************************************************/

// the result of just recognizing whole words
template<typename T>
void AndBitTablesHalfOpt ( T *table1, const T *table2, const size_t start, const size_t stop )
{
    size_t count = stop - start;
    size_t pos = start;
    
    if (stop < start)
        return;
    
    // align to word boundary
    while (count-- && (pos % (8*sizeof(T)) != 0)) {
        T word1 = table1[ pos / (8*sizeof(T)) ];
        T bit2 = table2[ pos / (8*sizeof(T)) ] & T(T(1) << (pos % (8*sizeof(T))));
        // clear bit in base, copy bit over from other set, and to get result with other bits still in place
        T result = ((word1 & ~(T(1) << (pos % (8*sizeof(T))))) | bit2) & word1;
        table1[ pos / (8*sizeof(T)) ] = result;
        pos++;
    }
    
    // whole words
    size_t index = pos / (8*sizeof(T));
    while ( count >= (8*sizeof(T)) ) {
        pos += (8*sizeof(T));
        count -= (8*sizeof(T));
        table1[ index ] &= table2[ index ];
        index++;
    }
    
    // remaining partial word
    while (count--) {
        T word1 = table1[ pos / (8*sizeof(T)) ];
        T bit2 = table2[ pos / (8*sizeof(T)) ] & T(T(1) << (pos % (8*sizeof(T))));
        // clear bit in base, copy bit over from other set, and to get result with other bits still in place
        T result = ((word1 & ~(T(1) << (pos % (8*sizeof(T))))) | bit2) & word1;
        table1[ pos / (8*sizeof(T)) ] = result;
        pos++;
    }

}

/******************************************************************************/

// the result of just recognizing whole words
template<typename T>
void OrBitTablesHalfOpt ( T *table1, const T *table2, const size_t start, const size_t stop )
{
    size_t count = stop - start;
    size_t pos = start;
    
    if (stop < start)
        return;
    
    // align to word boundary
    while (count-- && (pos % (8*sizeof(T)) != 0)) {
        T word1 = table1[ pos / (8*sizeof(T)) ];
        T bit2 = table2[ pos / (8*sizeof(T)) ] & T(T(1) << (pos % (8*sizeof(T))));
        // or with zeros will leave all other bits in place, only alter desired bit
        T result = (word1 | bit2);
        table1[ pos / (8*sizeof(T)) ] = result;
        pos++;
    }
    
    // whole words
    size_t index = pos / (8*sizeof(T));
    while ( count >= (8*sizeof(T)) ) {
        pos += (8*sizeof(T));
        count -= (8*sizeof(T));
        table1[ index ] |= table2[ index ];
        index++;
    }
    
    // remaining partial word
    while (count--) {
        T word1 = table1[ pos / (8*sizeof(T)) ];
        T bit2 = table2[ pos / (8*sizeof(T)) ] & T(T(1) << (pos % (8*sizeof(T))));
        // or with zeros will leave all other bits in place, only alter desired bit
        T result = (word1 | bit2);
        table1[ pos / (8*sizeof(T)) ] = result;
        pos++;
    }

}

/******************************************************************************/

// the result of just recognizing whole words
template<typename T>
void XorBitTablesHalfOpt ( T *table1, const T *table2, const size_t start, const size_t stop )
{
    size_t count = stop - start;
    size_t pos = start;
    
    if (stop < start)
        return;
    
    // align to word boundary
    while (count-- && (pos % (8*sizeof(T)) != 0)) {
        T word1 = table1[ pos / (8*sizeof(T)) ];
        T bit2 = table2[ pos / (8*sizeof(T)) ] & T(T(1) << (pos % (8*sizeof(T))));
        // xor with zeros will leave all other bits in place, only alter desired bit
        T result = (word1 ^ bit2);
        table1[ pos / (8*sizeof(T)) ] = result;
        pos++;
    }
    
    // whole words
    size_t index = pos / (8*sizeof(T));
    while ( count >= (8*sizeof(T)) ) {
        pos += (8*sizeof(T));
        count -= (8*sizeof(T));
        table1[ index ] ^= table2[ index ];
        index++;
    }
    
    // remaining partial word
    while (count--) {
        T word1 = table1[ pos / (8*sizeof(T)) ];
        T bit2 = table2[ pos / (8*sizeof(T)) ] & T(T(1) << (pos % (8*sizeof(T))));
        // xor with zeros will leave all other bits in place, only alter desired bit
        T result = (word1 ^ bit2);
        table1[ pos / (8*sizeof(T)) ] = result;
        pos++;
    }

}

/******************************************************************************/

// the result of just recognizing whole words
template<typename T>
void AndComplimentBitTablesHalfOpt ( T *table1, const T *table2, const size_t start, const size_t stop )
{
    size_t count = stop - start;
    size_t pos = start;
    
    if (stop < start)
        return;
    
    // align to word boundary
    while (count-- && (pos % (8*sizeof(T)) != 0)) {
        T word1 = table1[ pos / (8*sizeof(T)) ];
        T mask1 = T(T(1) << (pos % (8*sizeof(T))));
        T bit2 = table2[ pos / (8*sizeof(T)) ] & mask1;
        bit2 ^= mask1;    // invert the single bit
        // clear bit in base, copy bit over from other set, and to get result with other bits still in place
        T result = ((word1 & ~mask1) | bit2) & word1;
        table1[ pos / (8*sizeof(T)) ] = result;
        pos++;
    }
    
    // whole words
    size_t index = pos / (8*sizeof(T));
    while ( count >= (8*sizeof(T)) ) {
        pos += (8*sizeof(T));
        count -= (8*sizeof(T));
        table1[ index ] &= ~(table2[ index ]);
        index++;
    }
    
    // remaining partial word
    while (count--) {
        T word1 = table1[ pos / (8*sizeof(T)) ];
        T mask1 = T(T(1) << (pos % (8*sizeof(T))));
        T bit2 = table2[ pos / (8*sizeof(T)) ] & mask1;
        bit2 ^= mask1;    // invert the single bit
        // clear bit in base, copy bit over from other set, and to get result with other bits still in place
        T result = ((word1 & ~mask1) | bit2) & word1;
        table1[ pos / (8*sizeof(T)) ] = result;
        pos++;
    }

}

/******************************************************************************/

// optimizing the align and cleanup loops into single operations
template<typename T>
void AndBitTablesOptimized ( T *table1, const T *table2, const size_t start, const size_t stop )
{
    size_t count = stop - start;
    size_t pos = start;
    
    if (stop < start)
        return;
    
    // first partial word, if we will cross into another word, and if it is not already aligned
    size_t index = start / (8*sizeof(T));
    if ( ((stop / (8*sizeof(T))) > (start / (8*sizeof(T)))) && ((start % (8*sizeof(T))) != 0) ) {
        size_t bitstart = start % (8*sizeof(T));
        size_t adjust = ((8*sizeof(T)) - bitstart);
        T word1 = table1[ index ];
        T word2 = table2[ index ];
        T mask1 = T(~T(0) << bitstart);
        // copy overlapping bits from word2 to word1
        T temp = (word1 & mask1) | ( word2 & ~mask1);
        T result = temp & word1;
        table1[ index ] = result;
        count -= adjust;
        pos += adjust;
    }
    
    // whole words
    index = pos / (8*sizeof(T));
    while ( count >= (8*sizeof(T)) ) {
        pos += (8*sizeof(T));
        count -= (8*sizeof(T));
        table1[ index ] &= table2[ index ];
        index++;
    }
    
    // last partial word, if necessary
    if (count > 0) {
        //index = pos / (8*sizeof(T));    // already up to date from above
        T word1 = table1[ index ];
        T word2 = table2[ index ];
        T mask1 = T(~T(0) << (pos % (8*sizeof(T))));    // pos could be a multiple of 32 or equal to start!
        T mask2 = T(~T(0) << (stop % (8*sizeof(T))));
        T mask_range = mask1 & ~mask2;
        // copy overlapping bits from word2 to word1
        T temp = (word1 & mask_range) | ( word2 & ~mask_range);
        T result = temp & word1;
        table1[ index ] = result;
    }

}

/******************************************************************************/
/******************************************************************************/

// c = (a & b)
// a common operation for raster imaging systems
// this is "mark with a mask" or "mark with a halftone function"
template<typename T>
void StencilBitTables ( T *table1, const T *table2, const T *table3, const size_t start, const size_t stop )
{
    size_t count = stop - start;
    size_t pos = start;
    
    if (stop < start)
        return;
    
    // divide and mod will be by powers of 2:  they optimize to shift and mask
    while (count--) {
        T word1 = table1[ pos / (8*sizeof(T)) ];
        T word2 = table2[ pos / (8*sizeof(T)) ];
        T word3 = table3[ pos / (8*sizeof(T)) ];
        T bit2 = (word2 & word3) & T(T(1) << (pos % (8*sizeof(T))));
        // or with zeros will leave all other bits in place, only alter desired bit
        T result = (word1 | bit2);
        table1[ pos / (8*sizeof(T)) ] = result;
        pos++;
    }

}
/******************************************************************************/

// c |= (a & b)
// a common operation for raster imaging systems
// this is "mark with a mask" or "mark with a halftone function"
template<typename T>
void StencilBitTablesShift ( T *table1, const T *table2, const T *table3, const size_t start, const size_t stop )
{
    size_t count = stop - start;
    size_t pos = start;
    const int shift = shiftSizeByBytes[ sizeof(T) ];
    const T modMask = (1 << shift) - 1;
    
    if (stop < start)
        return;
    
    // divide and mod replaced with shift and mask
    while (count--) {
        size_t index = pos >> shift;
        T bitMask = T(T(1) << (pos & modMask));
        T word1 = table1[ index ];
        T word2 = table2[ index ];
        T word3 = table3[ index ];
        T bit2 = (word2 & word3) & bitMask;
        // or with zeros will leave all other bits in place, only alter desired bit
        T result = (word1 | bit2);
        table1[ index ] = result;
        pos++;
    }

}

/******************************************************************************/

// the result of just recognizing whole words
template<typename T>
void StencilBitTablesHalfOpt ( T *table1, const T *table2, const T *table3, const size_t start, const size_t stop )
{
    size_t count = stop - start;
    size_t pos = start;
    
    if (stop < start)
        return;
    
    // align to word boundary
    while (count-- && (pos % (8*sizeof(T)) != 0)) {
        T word1 = table1[ pos / (8*sizeof(T)) ];
        T word2 = table2[ pos / (8*sizeof(T)) ];
        T word3 = table3[ pos / (8*sizeof(T)) ];
        T bit2 = (word2 & word3) & T(T(1) << (pos % (8*sizeof(T))));
        // or with zeros will leave all other bits in place, only alter desired bit
        T result = (word1 | bit2);
        table1[ pos / (8*sizeof(T)) ] = result;
        pos++;
    }
    
    // whole words
    size_t index = pos / (8*sizeof(T));
    while ( count >= (8*sizeof(T)) ) {
        pos += (8*sizeof(T));
        count -= (8*sizeof(T));
        table1[ index ] |= (table2[ index ] & table3[index]);
        index++;
    }
    
    // remaining partial word
    while (count--) {
        T word1 = table1[ pos / (8*sizeof(T)) ];
        T word2 = table2[ pos / (8*sizeof(T)) ];
        T word3 = table3[ pos / (8*sizeof(T)) ];
        T bit2 = (word2 & word3) & T(T(1) << (pos % (8*sizeof(T))));
        // or with zeros will leave all other bits in place, only alter desired bit
        T result = (word1 | bit2);
        table1[ pos / (8*sizeof(T)) ] = result;
        pos++;
    }

}

/******************************************************************************/
/******************************************************************************/

// This is possibly the worst way to count bits, but I've seen it done
template<typename T>
size_t CountBits ( T *table, const size_t start, const size_t stop )
{
    size_t count = stop - start;
    size_t pos = start;
    size_t result = 0;
    
    if (stop < start)
        return 0;
    
    // divide and mod will be by powers of 2:  they optimize to shift and mask
    while (count--) {
        T bitValue = table[ pos / (8*sizeof(T)) ] & T(T(1) << (pos % (8*sizeof(T))));
        if (bitValue != 0)
            result++;
        pos++;
    }
    
    return result;
}
/******************************************************************************/

// This is possibly the worst way to count bits, but I've seen it done
template<typename T>
size_t CountBitsShift ( T *table, const size_t start, const size_t stop )
{
    size_t count = stop - start;
    size_t pos = start;
    size_t result = 0;
    const int shift = shiftSizeByBytes[ sizeof(T) ];
    const T modMask = (T(1) << shift) - 1;
    
    if (stop < start)
        return 0;
    
    // divide and mod replaced with shift and mask
    while (count--) {
        size_t index = pos >> shift;
        T bitMask = T(T(1) << (pos & modMask));
        T bitValue = table[ index ] & bitMask;
        if (bitValue != 0)
            result++;
        pos++;
    }
    
    return result;
}

/******************************************************************************/

// NOTE - ccox - I've seen this loop used many times (or 0x80 >>= 1)
// few compilers seem to recognize this as an unrollable loop of constant length
inline
size_t BitsInByteLoop(unsigned char byte) {
    size_t count = 0;
    unsigned char mask = 1;
    
    do {
        if (byte & mask)    count++;
        mask <<= 1;
    } while (mask);
    
    return count;
}

// treat the array as bytes, count bits within bytes
template<typename T>
size_t CountBitsByteLoop ( T *table, const size_t start, const size_t stop )
{
    size_t count = stop - start;
    size_t pos = start;
    size_t result = 0;
    
    if (stop < start)
        return 0;

    // align to byte boundary
    while (count && ((pos % 8) != 0)) {
        T bitValue = table[ pos / (8*sizeof(T)) ] & T(T(1) << (pos % (8*sizeof(T))));
        if (bitValue != 0)
            result++;
        pos++;
        count--;
    }
    
    // whole bytes
    uint8_t *byteTable = (uint8_t *)table;
    size_t index = pos / 8;
    while ( count >= 8 ) {
        pos += 8;
        count -= 8;
        result += BitsInByteLoop( byteTable[index] );
        index++;
    }

    // remaining partial byte (can't read over end of array)
    while (count--) {
        uint8_t bitValue = byteTable[ pos / 8 ] & uint8_t(uint8_t(1) << (pos % 8));
        if (bitValue != 0)
            result++;
        pos++;
    }
    
    return result;
}

/******************************************************************************/

// NOTE - ccox - I've seen this loop used many times (or 0x01 <<= 1)
// few compilers seem to recognize this as an unrollable loop of constant length
inline
size_t BitsInByteLoop2(unsigned char byte) {
    size_t count = 0;
    unsigned char mask = 0x80;
    
    do {
        if (byte & mask)    count++;
        mask >>= 1;
    } while (mask);
    
    return count;
}

// treat the array as bytes, count bits within bytes
template<typename T>
size_t CountBitsByteLoop2 ( T *table, const size_t start, const size_t stop )
{
    size_t count = stop - start;
    size_t pos = start;
    size_t result = 0;
    
    if (stop < start)
        return 0;

    // align to byte boundary
    while (count && ((pos % 8) != 0)) {
        T bitValue = table[ pos / (8*sizeof(T)) ] & T(T(1) << (pos % (8*sizeof(T))));
        if (bitValue != 0)
            result++;
        pos++;
        count--;
    }
    
    // whole bytes
    uint8_t *byteTable = (uint8_t *)table;
    size_t index = pos / 8;
    while ( count >= 8 ) {
        pos += 8;
        count -= 8;
        result += BitsInByteLoop2( byteTable[index] );
        index++;
    }

    // remaining partial byte (can't read over end of array)
    while (count--) {
        uint8_t bitValue = byteTable[ pos / 8 ] & uint8_t(uint8_t(1) << (pos % 8));
        if (bitValue != 0)
            result++;
        pos++;
    }
    
    return result;
}

/******************************************************************************/

inline
size_t BitsInByteUnrolled(unsigned char byte) {
    size_t count = 0;
    if (byte & (1 << 0))    count++;
    if (byte & (1 << 1))    count++;
    if (byte & (1 << 2))    count++;
    if (byte & (1 << 3))    count++;
    if (byte & (1 << 4))    count++;
    if (byte & (1 << 5))    count++;
    if (byte & (1 << 6))    count++;
    if (byte & (1 << 7))    count++;
    return count;
}

// treat the array as bytes, count bits within bytes
template<typename T>
size_t CountBitsByteDirect ( T *table, const size_t start, const size_t stop )
{
    size_t count = stop - start;
    size_t pos = start;
    size_t result = 0;
    
    if (stop < start)
        return 0;
    
    // align to byte boundary
    while (count && ((pos % 8) != 0)) {
        T bitValue = table[ pos / (8*sizeof(T)) ] & T(T(1) << (pos % (8*sizeof(T))));
        if (bitValue != 0)
            result++;
        pos++;
        count--;
    }
    
    // whole bytes
    uint8_t *byteTable = (uint8_t *)table;
    size_t index = pos / 8;
    while ( count >= 8 ) {
        pos += 8;
        count -= 8;
        result += BitsInByteUnrolled( byteTable[index] );
        index++;
    }
    
    // remaining partial byte (can't read over end of array)
    while (count--) {
        uint8_t bitValue = byteTable[ pos / 8 ] & uint8_t(uint8_t(1) << (pos % 8));
        if (bitValue != 0)
            result++;
        pos++;
    }
    
    return result;
}

/******************************************************************************/

inline
size_t BitsInByteUnrolled2(unsigned char byte) {
    size_t count0 = 0, count1 = 0;
    if (byte & (1 << 0))    count0++;
    if (byte & (1 << 1))    count1++;
    if (byte & (1 << 2))    count0++;
    if (byte & (1 << 3))    count1++;
    if (byte & (1 << 4))    count0++;
    if (byte & (1 << 5))    count1++;
    if (byte & (1 << 6))    count0++;
    if (byte & (1 << 7))    count1++;
    return count0 + count1;
}

// treat the array as bytes, count bits within bytes
template<typename T>
size_t CountBitsByteDirect2 ( T *table, const size_t start, const size_t stop )
{
    size_t count = stop - start;
    size_t pos = start;
    size_t result = 0;
    
    if (stop < start)
        return 0;
    
    // align to byte boundary
    while (count && ((pos % 8) != 0)) {
        T bitValue = table[ pos / (8*sizeof(T)) ] & T(T(1) << (pos % (8*sizeof(T))));
        if (bitValue != 0)
            result++;
        pos++;
        count--;
    }
    
    // whole bytes
    uint8_t *byteTable = (uint8_t *)table;
    size_t index = pos / 8;
    while ( count >= 8 ) {
        pos += 8;
        count -= 8;
        result += BitsInByteUnrolled2( byteTable[index] );
        index++;
    }
    
    // remaining partial byte (can't read over end of array)
    while (count--) {
        uint8_t bitValue = byteTable[ pos / 8 ] & uint8_t(uint8_t(1) << (pos % 8));
        if (bitValue != 0)
            result++;
        pos++;
    }
    
    return result;
}

/******************************************************************************/

static size_t bitCountTable[256];
static bool bitCountInitialized = false;

void InitializeBitCountTable() {
    if (bitCountInitialized)
        return;
    
    int i;
    for (i = 0; i < 256; ++i) {
        bitCountTable[i] = BitsInByteUnrolled( (unsigned char) i );
    }
    
    bitCountInitialized = true;
}

/******************************************************************************/

// treat the array as bytes, count bits within bytes using a prebuilt table
template<typename T>
size_t CountBitsByteTable ( T *table, const size_t start, const size_t stop )
{
    size_t count = stop - start;
    size_t pos = start;
    size_t result = 0;
    
    if (stop < start)
        return 0;
    
    InitializeBitCountTable();
    
    // align to byte boundary
    while (count && ((pos % 8) != 0)) {
        T bitValue = table[ pos / (8*sizeof(T)) ] & T(T(1) << (pos % (8*sizeof(T))));
        if (bitValue != 0)
            result++;
        pos++;
        count--;
    }
    
    // whole bytes
    uint8_t *byteTable = (uint8_t *)table;
    size_t index = pos / 8;
    while ( count >= 8 ) {
        pos += 8;
        count -= 8;
        result += bitCountTable[ byteTable[index] ];
        index++;
    }

    
    // remaining partial byte (can't read over end of array)
    while (count--) {
        uint8_t bitValue = byteTable[ pos / 8 ] & uint8_t(uint8_t(1) << (pos % 8));
        if (bitValue != 0)
            result++;
        pos++;
    }
    
    return result;
}

/******************************************************************************/

inline
uint32_t CountBitsIntParallel( uint32_t value ) {
    uint32_t count = 0;
    count = value - ((value >> 1) & 0x55555555);
    count = ((count >> 2) & 0x33333333) + (count & 0x33333333);
    count = ((count >> 4) + count) & 0x0F0F0F0F;
    count = ((count >> 8) + count) & 0x00FF00FF;
    count = ((count >> 16) + count) & 0x0000FFFF;
    return count;
}

// treat the array as 32 bit ints, count using bit ops
template<typename T>
size_t CountBits32Parallel ( T *table, const size_t start, const size_t stop )
{
    size_t count = stop - start;
    size_t pos = start;
    size_t result = 0;
    
    if (stop < start)
        return 0;
    
    InitializeBitCountTable();
    
    // align to int boundary
    while (count && ((pos % 32) != 0)) {
        T bitValue = table[ pos / (8*sizeof(T)) ] & T(T(1) << (pos % (8*sizeof(T))));
        if (bitValue != 0)
            result++;
        pos++;
        count--;
    }
    
    // whole ints
    uint32_t *intTable = (uint32_t *)table;
    size_t index = pos / 32;
    while ( count >= 32 ) {
        pos += 32;
        count -= 32;
        result += CountBitsIntParallel( intTable[index] );
        index++;
    }
    
    // remaining partial int
    // can't use int table without chance of reading over end of array
    while (count--) {
        T bitValue = table[ pos / (8*sizeof(T)) ] & T(T(1) << (pos % (8*sizeof(T))));
        if (bitValue != 0)
            result++;
        pos++;
    }
    
    return result;
}

/******************************************************************************/

inline
uint32_t CountBitsIntMultiply( uint32_t value ) {
    uint32_t count = 0;
    value = value - ((value >> 1) & 0x55555555);
    value = (value & 0x33333333) + ((value >> 2) & 0x33333333);
    count = ((value + (value >> 4) & 0x0F0F0F0F) * 0x01010101) >> 24;
    return count;
}

// treat the array as 32 bit ints, count using bit ops
template<typename T>
size_t CountBits32Multiply ( T *table, const size_t start, const size_t stop )
{
    size_t count = stop - start;
    size_t pos = start;
    size_t result = 0;
    
    if (stop < start)
        return 0;
    
    InitializeBitCountTable();
    
    // align to int boundary
    while (count && ((pos % 32) != 0)) {
        T bitValue = table[ pos / (8*sizeof(T)) ] & T(T(1) << (pos % (8*sizeof(T))));
        if (bitValue != 0)
            result++;
        pos++;
        count--;
    }
    
    // whole ints
    uint32_t *intTable = (uint32_t *)table;
    size_t index = pos / 32;
    while ( count >= 32 ) {
        pos += 32;
        count -= 32;
        result += CountBitsIntMultiply( intTable[index] );
        index++;
    }
    
    // remaining partial int
    // can't use int table without chance of reading over end of array
    while (count--) {
        T bitValue = table[ pos / (8*sizeof(T)) ] & T(T(1) << (pos % (8*sizeof(T))));
        if (bitValue != 0)
            result++;
        pos++;
    }
    
    return result;
}

/******************************************************************************/
/******************************************************************************/

template <typename T>
void check_bitset(T* first, const size_t start, const size_t stop, size_t expected_value, const char *label)
{
    // count
    size_t count = CountBitsShift<T>( first, start, stop );
    
    // compare
    if (count != expected_value)
        printf("test %s failed, got %zu bits instead of %zu\n", label, count, expected_value);
}

/******************************************************************************/

template <typename T, typename WorkFunction>
void test_setbits(T* first, const size_t start, const size_t stop, size_t expected_value, WorkFunction work_function, const char *label) {
  int i;
  
  start_timer();
  
  for(i = 0; i < iterations; ++i) {
    work_function( first, start, stop );
  }
    
  record_result( timer(), label );
  
  check_bitset<T>(first,start,stop,expected_value,label);
}

/******************************************************************************/

template <typename T, typename WorkFunction>
void test_mergebits(T* table1, const T* table2, const size_t start, const size_t stop, size_t expected_value, WorkFunction work_function, const char *label) {
  int i;
  
  start_timer();
  
  for(i = 0; i < iterations; ++i) {
    work_function( table1, table2, start, stop );
  }
  
  record_result( timer(), label );
  
  check_bitset<T>(table1,start,stop,expected_value,label);
}

/******************************************************************************/

template <typename T, typename WorkFunction>
void test_blitbits(T* table1, const T* table2, const T*table3, const size_t start, const size_t stop, size_t expected_value, WorkFunction work_function, const char *label) {
  int i;
  
  start_timer();
  
  for(i = 0; i < iterations; ++i) {
    work_function( table1, table2, table3, start, stop );
  }
  
  record_result( timer(), label );
  
  check_bitset<T>(table1,start,stop,expected_value,label);
}

/******************************************************************************/

void check_bit_count(size_t count, const char *label) {
    if (count != (BITSIZE - 3 - 3))
        printf("test %s failed, got %zu bits\n", label, count);
}

/******************************************************************************/

template <typename T, typename WorkFunction>
void test_countbits(T* first, const size_t start, const size_t stop, WorkFunction work_function, const char *label) {
  int i;
  size_t count = 0;
  
  start_timer();
  
  for(i = 0; i < iterations; ++i) {
    count = work_function( first, start, stop );
  }

  check_bit_count(count,label);
  
  record_result( timer(), label );

}

/******************************************************************************/
/******************************************************************************/

// stl appears to call through to memset
void SetBitsStd ( testBitset &table )
{
    table.set();
}

/******************************************************************************/

// set each bit in a loop
void SetBitsStdLoop ( testBitset &table )
{
    for ( size_t pos = 0; pos < table.size(); ++pos)
        table.set(pos);
}

/******************************************************************************/

// set each bit in a loop
void SetBitsStdLoop2 ( testBitset &table )
{
    for ( size_t pos = 0; pos < table.size(); ++pos)
        table[pos] = 1;
}

/******************************************************************************/

// stl appears to call through to memset
void ClearBitsStd ( testBitset &table )
{
    table.reset();
}

/******************************************************************************/

// clear each bit in a loop
void ClearBitsStdLoop ( testBitset &table )
{
    for ( size_t pos = 0; pos < table.size(); ++pos)
        table.reset(pos);
}

/******************************************************************************/

// clear each bit in a loop
void ClearBitsStdLoop2 ( testBitset &table )
{
    for ( size_t pos = 0; pos < table.size(); ++pos)
        table[pos] = 0;
}

/******************************************************************************/

// stl works on 64 bit words
void InvertBitsStd ( testBitset &table )
{
    table.flip();
}

/******************************************************************************/

// clear each bit in a loop
void InvertBitsStdLoop ( testBitset &table )
{
    for ( size_t pos = 0; pos < table.size(); ++pos)
        table.flip(pos);
}

/******************************************************************************/

// invert each bit in a loop
void InvertBitsStdLoop2 ( testBitset &table )
{
    for ( size_t pos = 0; pos < table.size(); ++pos)
        table[pos] = !table[pos];
}

/******************************************************************************/

// stl works on 64 bit words (and doesn't try to mask unused bits)
void AndBitTablesStd ( testBitset &table1, const testBitset &table2 )
{
    table1 &= table2;
}

/******************************************************************************/

// and each bit in a loop, logic op
void AndBitTablesStdLoop ( testBitset &table1, const testBitset &table2 )
{
    for ( size_t pos = 0; pos < table1.size(); ++pos)
        table1[pos] = table1[pos] && table2[pos];
        //table1[pos] &= table2[pos];        // accessor doesn't support &=
}

/******************************************************************************/

// and each bit in a loop, bit op
void AndBitTablesStdLoop2 ( testBitset &table1, const testBitset &table2 )
{
    for ( size_t pos = 0; pos < table1.size(); ++pos)
        table1[pos] = table1[pos] & table2[pos];
        //table1[pos] &= table2[pos];        // accessor doesn't support &=
}

/******************************************************************************/

// stl works on 64 bit words (and doesn't try to mask unused bits)
void OrBitTablesStd ( testBitset &table1, const testBitset &table2 )
{
    table1 |= table2;
}

/******************************************************************************/

// or each bit in a loop, logic op
void OrBitTablesStdLoop ( testBitset &table1, const testBitset &table2 )
{
    for ( size_t pos = 0; pos < table1.size(); ++pos)
        table1[pos] = table1[pos] || table2[pos];
        //table1[pos] |= table2[pos];        // accessor doesn't support |=
}

/******************************************************************************/

// or each bit in a loop, bit op
void OrBitTablesStdLoop2 ( testBitset &table1, const testBitset &table2 )
{
    for ( size_t pos = 0; pos < table1.size(); ++pos)
        table1[pos] = table1[pos] | table2[pos];
        //table1[pos] |= table2[pos];        // accessor doesn't support |=
}

/******************************************************************************/

// stl works on 64 bit words (and doesn't try to mask unused bits)
void XorBitTablesStd ( testBitset &table1, const testBitset &table2 )
{
    table1 ^= table2;
}

/******************************************************************************/

// xor each bit in a loop
void XorBitTablesStdLoop ( testBitset &table1, const testBitset &table2 )
{
    for ( size_t pos = 0; pos < table1.size(); ++pos)
        table1[pos] = table1[pos] ^ table2[pos];
        //table1[pos] ^= table2[pos];        // accessor doesn't support ^=
}

/******************************************************************************/

void AndComplimentBitTablesStd ( testBitset &table1, const testBitset &table2 )
{
    table1 &= ~table2;
}

/******************************************************************************/

// and compliment, logic ops
void AndComplimentBitTablesStdLoop ( testBitset &table1, const testBitset &table2 )
{
    for ( size_t pos = 0; pos < table1.size(); ++pos)
        table1[pos] = table1[pos] && !table2[pos];
        //table1[pos] &= !table2[pos];        // accessor doesn't support &=
}

/******************************************************************************/

// and compliment, bit ops
void AndComplimentBitTablesStdLoop2 ( testBitset &table1, const testBitset &table2 )
{
    for ( size_t pos = 0; pos < table1.size(); ++pos)
        table1[pos] = table1[pos] & ~table2[pos];
        //table1[pos] &= !table2[pos];        // accessor doesn't support &=
}

/******************************************************************************/

void StencilBitTablesStd ( testBitset &table1, const testBitset &table2, const testBitset &table3 )
{
    table1 |= (table2 & table3);
}

/******************************************************************************/

// stencil, logic ops
void StencilBitTablesStdLoop ( testBitset &table1, const testBitset &table2, const testBitset &table3 )
{
    for ( size_t pos = 0; pos < table1.size(); ++pos)
        table1[pos] = table1[pos] || (table2[pos] && table3[pos]);
        //table1[pos] |= (table2[pos] & table3[pos]);        // accessor doesn't support |=
}

/******************************************************************************/

// stencil, bit ops
void StencilBitTablesStdLoop2 ( testBitset &table1, const testBitset &table2, const testBitset &table3 )
{
    for ( size_t pos = 0; pos < table1.size(); ++pos)
        table1[pos] = table1[pos] | (table2[pos] & table3[pos]);
        //table1[pos] |= (table2[pos] & table3[pos]);        // accessor doesn't support |=
}

/******************************************************************************/
/******************************************************************************/

void check_bitsetStd( testBitset first, size_t expected_value, const char *label)
{
    // count
    size_t count = first.count();
    
    // compare
    if (count != expected_value)
        printf("test %s failed, got %zu bits instead of %zu\n", label, count, expected_value);
}

/******************************************************************************/

template <typename WorkFunction>
void test_setbitsStd( testBitset &first, size_t expected_value, WorkFunction work_function, const char *label) {
  int i;
  
  start_timer();
  
  for(i = 0; i < iterations; ++i) {
    work_function( first );
  }

  record_result( timer(), label );
  
  check_bitsetStd(first,expected_value,label);
}

/******************************************************************************/

template <typename WorkFunction>
void test_mergebitsStd( testBitset &table1, const testBitset &table2, size_t expected_value, WorkFunction work_function, const char *label) {
  int i;
  
  start_timer();
  
  for(i = 0; i < iterations; ++i) {
    work_function( table1, table2 );
  }
  
  record_result( timer(), label );
  
  check_bitsetStd(table1,expected_value,label);
}

/******************************************************************************/

template <typename WorkFunction>
void test_blitbitsStd( testBitset &table1, const testBitset &table2, const testBitset &table3, size_t expected_value, WorkFunction work_function, const char *label) {
  int i;
  
  start_timer();
  
  for(i = 0; i < iterations; ++i) {
    work_function( table1, table2, table3 );
  }
  
  record_result( timer(), label );
  
  check_bitsetStd(table1,expected_value,label);
}

/******************************************************************************/

void check_bit_countStd(size_t count, const char *label) {
    if (count != (BITSIZE - 3 - 3))
        printf("test %s failed, got %zu bits\n", label, count);
}

/******************************************************************************/

// stl uses sloppy bit iterators and doesn't correctly special case counting the entire array
// then uses whole words inside a general counting function, using count bit intrinsic
// so, sorta good, but not optimal
void test_countbitsStd(testBitset &first, const char *label) {
  int i;
  size_t count = 0;
  
  start_timer();
  
  for(i = 0; i < iterations; ++i) {
    count = first.count();
  }

  check_bit_count(count,label);
  
  record_result( timer(), label );

}

/******************************************************************************/

void test_countbitsStdLoop(testBitset &first, const char *label) {
  int i;
  size_t count = 0;
  
  start_timer();
  
  for(i = 0; i < iterations; ++i) {
    count = 0;
    for ( size_t pos = 0; pos < first.size(); ++pos)
        if (first[pos])
            count++;
  }

  check_bit_count(count,label);
  
  record_result( timer(), label );

}

/******************************************************************************/
/******************************************************************************/


int main(int argc, char** argv) {
    
    // output command for documentation:
    int i;
    for (i = 0; i < argc; ++i)
        printf("%s ", argv[i] );
    printf("\n");

    if (argc > 1) iterations = atoi(argv[1]);
    if (argc > 2) init_value = atoi(argv[2]);



// uint8_t
    size_t setCount8 = (BITSIZE+7)/(8*sizeof(uint8_t));
    ::fill(data8unsigned, data8unsigned+setCount8, uint8_t(init_value));
    size_t valueExpected8 = CountBitsShift( data8unsigned, 3, BITSIZE-3 );
    ::fill(data8unsigned2, data8unsigned2+setCount8, uint8_t(init_value));
    ::fill(data8unsigned3, data8unsigned3+setCount8, uint8_t(0));
    size_t invertExpected8 = (iterations&1) ? (BITSIZE-6-valueExpected8) : valueExpected8;

    test_setbits( data8unsigned, 3, BITSIZE - 3, (BITSIZE-6), SetBitsOptimized<uint8_t>, "uint8_t setbits optimized");
    test_setbits( data8unsigned, 3, BITSIZE - 3, (BITSIZE-6), SetBitsHalfOpt<uint8_t>, "uint8_t setbits half-opt");
    test_setbits( data8unsigned, 3, BITSIZE - 3, (BITSIZE-6), SetBitsShift<uint8_t>, "uint8_t setbits shift");
    test_setbits( data8unsigned, 3, BITSIZE - 3, (BITSIZE-6), SetBits<uint8_t>, "uint8_t setbits");
    
    test_setbits( data8unsigned, 3, BITSIZE - 3, 0, ClearBitsHalfOpt<uint8_t>, "uint8_t clearbits half-opt");
    test_setbits( data8unsigned, 3, BITSIZE - 3, 0, ClearBitsShift<uint8_t>, "uint8_t clearbits shift");
    test_setbits( data8unsigned, 3, BITSIZE - 3, 0, ClearBits<uint8_t>, "uint8_t clearbits");
    
    test_setbits( data8unsigned2, 3, BITSIZE - 3, invertExpected8, InvertBitsHalfOpt<uint8_t>, "uint8_t invertbits half-opt");
    test_setbits( data8unsigned2, 3, BITSIZE - 3, invertExpected8, InvertBitsShift<uint8_t>, "uint8_t invertbits shift");
    test_setbits( data8unsigned2, 3, BITSIZE - 3, invertExpected8, InvertBits<uint8_t>, "uint8_t invertbits");
    
    ::fill(data8unsigned, data8unsigned+setCount8, uint8_t(init_value));
    ::fill(data8unsigned2, data8unsigned2+setCount8, uint8_t(init_value));
    test_mergebits( data8unsigned, data8unsigned2, 3, BITSIZE - 3, valueExpected8, AndBitTablesOptimized<uint8_t>, "uint8_t and bit tables optimized");
    test_mergebits( data8unsigned, data8unsigned2, 3, BITSIZE - 3, valueExpected8, AndBitTablesHalfOpt<uint8_t>, "uint8_t and bit tables half-opt");
    test_mergebits( data8unsigned, data8unsigned2, 3, BITSIZE - 3, valueExpected8, AndBitTablesShift<uint8_t>, "uint8_t and bit tables shift");
    test_mergebits( data8unsigned, data8unsigned2, 3, BITSIZE - 3, valueExpected8, AndBitTables<uint8_t>, "uint8_t and bit tables");
    
    test_mergebits( data8unsigned, data8unsigned2, 3, BITSIZE - 3, valueExpected8, OrBitTablesHalfOpt<uint8_t>, "uint8_t or bit tables half-opt");
    test_mergebits( data8unsigned, data8unsigned2, 3, BITSIZE - 3, valueExpected8, OrBitTablesShift<uint8_t>, "uint8_t or bit tables shift");
    test_mergebits( data8unsigned, data8unsigned2, 3, BITSIZE - 3, valueExpected8, OrBitTables<uint8_t>, "uint8_t or bit tables");
    
    test_mergebits( data8unsigned, data8unsigned2, 3, BITSIZE - 3, invertExpected8, XorBitTablesHalfOpt<uint8_t>, "uint8_t xor bit tables half-opt");
    test_mergebits( data8unsigned, data8unsigned2, 3, BITSIZE - 3, invertExpected8, XorBitTablesShift<uint8_t>, "uint8_t xor bit tables shift");
    test_mergebits( data8unsigned, data8unsigned2, 3, BITSIZE - 3, invertExpected8, XorBitTables<uint8_t>, "uint8_t xor bit tables");

    test_mergebits( data8unsigned, data8unsigned2, 3, BITSIZE - 3, 0, AndComplimentBitTablesHalfOpt<uint8_t>, "uint8_t and compliment bit tables half-opt");
    test_mergebits( data8unsigned, data8unsigned2, 3, BITSIZE - 3, 0, AndComplimentBitTablesShift<uint8_t>, "uint8_t and compliment bit tables shift");
    test_mergebits( data8unsigned, data8unsigned2, 3, BITSIZE - 3, 0, AndComplimentBitTables<uint8_t>, "uint8_t and compliment bit tables");

    ::fill(data8unsigned, data8unsigned+setCount8, uint8_t(init_value));
    ::fill(data8unsigned2, data8unsigned2+setCount8, uint8_t(init_value));
    test_blitbits( data8unsigned3, data8unsigned2, data8unsigned, 3, BITSIZE - 3, valueExpected8, StencilBitTablesHalfOpt<uint8_t>, "uint8_t stencil bit tables half-opt");
    test_blitbits( data8unsigned3, data8unsigned2, data8unsigned, 3, BITSIZE - 3, valueExpected8, StencilBitTablesShift<uint8_t>, "uint8_t stencil bit tables shift");
    test_blitbits( data8unsigned3, data8unsigned2, data8unsigned, 3, BITSIZE - 3, valueExpected8, StencilBitTables<uint8_t>, "uint8_t stencil bit tables");

    SetBitsOptimized( data8unsigned, 3, BITSIZE- 3);
    test_countbits( data8unsigned, 3, BITSIZE - 3, CountBits32Multiply<uint8_t>, "uint8_t count bits 32Multiply");
    test_countbits( data8unsigned, 3, BITSIZE - 3, CountBits32Parallel<uint8_t>, "uint8_t count bits 32Parallel");
    test_countbits( data8unsigned, 3, BITSIZE - 3, CountBitsByteTable<uint8_t>, "uint8_t count bits ByteTable");
    test_countbits( data8unsigned, 3, BITSIZE - 3, CountBitsByteDirect<uint8_t>, "uint8_t count bits ByteDirect");
    test_countbits( data8unsigned, 3, BITSIZE - 3, CountBitsByteDirect2<uint8_t>, "uint8_t count bits ByteDirect2");
    test_countbits( data8unsigned, 3, BITSIZE - 3, CountBitsByteLoop<uint8_t>, "uint8_t count bits ByteLoop");
    test_countbits( data8unsigned, 3, BITSIZE - 3, CountBitsByteLoop2<uint8_t>, "uint8_t count bits ByteLoop2");
    test_countbits( data8unsigned, 3, BITSIZE - 3, CountBitsShift<uint8_t>, "uint8_t count bits shift");
    test_countbits( data8unsigned, 3, BITSIZE - 3, CountBits<uint8_t>, "uint8_t count bits");

    summarize("uint8_t bitarrays", BITSIZE, iterations, kDontShowGMeans, kDontShowPenalty );



// uint16_t
    size_t setCount16 = (BITSIZE+15)/(8*sizeof(uint16_t));
    ::fill(data16unsigned, data16unsigned+setCount16, uint16_t(init_value));
    size_t valueExpected16 = CountBitsShift( data16unsigned, 3, BITSIZE-3 );
    ::fill(data16unsigned2, data16unsigned2+setCount16, uint16_t(init_value));
    ::fill(data16unsigned3, data16unsigned3+setCount16, uint16_t(0));
    size_t invertExpected16 = valueExpected16;
    
    test_setbits( data16unsigned, 3, BITSIZE - 3, (BITSIZE-6), SetBitsOptimized<uint16_t>, "uint16_t setbits optimized");
    test_setbits( data16unsigned, 3, BITSIZE - 3, (BITSIZE-6), SetBitsHalfOpt<uint16_t>, "uint16_t setbits half-opt");
    test_setbits( data16unsigned, 3, BITSIZE - 3, (BITSIZE-6), SetBitsShift<uint16_t>, "uint16_t setbits shift");
    test_setbits( data16unsigned, 3, BITSIZE - 3, (BITSIZE-6), SetBits<uint16_t>, "uint16_t setbits");
    
    test_setbits( data16unsigned, 3, BITSIZE - 3, 0, ClearBitsHalfOpt<uint16_t>, "uint16_t clearbits half-opt");
    test_setbits( data16unsigned, 3, BITSIZE - 3, 0, ClearBitsShift<uint16_t>, "uint16_t clearbits shift");
    test_setbits( data16unsigned, 3, BITSIZE - 3, 0, ClearBits<uint16_t>, "uint16_t clearbits");
    
    test_setbits( data16unsigned2, 3, BITSIZE - 3, invertExpected16, InvertBitsHalfOpt<uint16_t>, "uint16_t invertbits half-opt");
    test_setbits( data16unsigned2, 3, BITSIZE - 3, invertExpected16, InvertBitsShift<uint16_t>, "uint16_t invertbits shift");
    test_setbits( data16unsigned2, 3, BITSIZE - 3, invertExpected16, InvertBits<uint16_t>, "uint16_t invertbits");
    
    ::fill(data16unsigned, data16unsigned+setCount16, uint16_t(init_value));
    ::fill(data16unsigned2, data16unsigned2+setCount16, uint16_t(init_value));
    test_mergebits( data16unsigned, data16unsigned2, 3, BITSIZE - 3, valueExpected16, AndBitTablesOptimized<uint16_t>, "uint16_t and bit tables optimized");
    test_mergebits( data16unsigned, data16unsigned2, 3, BITSIZE - 3, valueExpected16, AndBitTablesHalfOpt<uint16_t>, "uint16_t and bit tables half-opt");
    test_mergebits( data16unsigned, data16unsigned2, 3, BITSIZE - 3, valueExpected16, AndBitTablesShift<uint16_t>, "uint16_t and bit tables shift");
    test_mergebits( data16unsigned, data16unsigned2, 3, BITSIZE - 3, valueExpected16, AndBitTables<uint16_t>, "uint16_t and bit tables");
    
    test_mergebits( data16unsigned, data16unsigned2, 3, BITSIZE - 3, valueExpected16, OrBitTablesHalfOpt<uint16_t>, "uint16_t or bit tables half-opt");
    test_mergebits( data16unsigned, data16unsigned2, 3, BITSIZE - 3, valueExpected16, OrBitTablesShift<uint16_t>, "uint16_t or bit tables shift");
    test_mergebits( data16unsigned, data16unsigned2, 3, BITSIZE - 3, valueExpected16, OrBitTables<uint16_t>, "uint16_t or bit tables");
    
    test_mergebits( data16unsigned, data16unsigned2, 3, BITSIZE - 3, invertExpected16, XorBitTablesHalfOpt<uint16_t>, "uint16_t xor bit tables half-opt");
    test_mergebits( data16unsigned, data16unsigned2, 3, BITSIZE - 3, invertExpected16, XorBitTablesShift<uint16_t>, "uint16_t xor bit tables shift");
    test_mergebits( data16unsigned, data16unsigned2, 3, BITSIZE - 3, invertExpected16, XorBitTables<uint16_t>, "uint16_t xor bit tables");

    test_mergebits( data16unsigned, data16unsigned2, 3, BITSIZE - 3, 0, AndComplimentBitTablesHalfOpt<uint16_t>, "uint16_t and compliment bit tables half-opt");
    test_mergebits( data16unsigned, data16unsigned2, 3, BITSIZE - 3, 0, AndComplimentBitTablesShift<uint16_t>, "uint16_t and compliment bit tables shift");
    test_mergebits( data16unsigned, data16unsigned2, 3, BITSIZE - 3, 0, AndComplimentBitTables<uint16_t>, "uint16_t and compliment bit tables");

    ::fill(data16unsigned, data16unsigned+setCount16, uint16_t(init_value));
    ::fill(data16unsigned2, data16unsigned2+setCount16, uint16_t(init_value));
    test_blitbits( data16unsigned3, data16unsigned2, data16unsigned, 3, BITSIZE - 3, valueExpected16, StencilBitTablesHalfOpt<uint16_t>, "uint16_t stencil bit tables half-opt");
    test_blitbits( data16unsigned3, data16unsigned2, data16unsigned, 3, BITSIZE - 3, valueExpected16, StencilBitTablesShift<uint16_t>, "uint16_t stencil bit tables shift");
    test_blitbits( data16unsigned3, data16unsigned2, data16unsigned, 3, BITSIZE - 3, valueExpected16, StencilBitTables<uint16_t>, "uint16_t stencil bit tables");

    SetBitsOptimized( data16unsigned, 3, BITSIZE- 3);
    test_countbits( data16unsigned, 3, BITSIZE - 3, CountBits32Multiply<uint16_t>, "uint16_t count bits 32Multiply");
    test_countbits( data16unsigned, 3, BITSIZE - 3, CountBits32Parallel<uint16_t>, "uint16_t count bits 32Parallel");
    test_countbits( data16unsigned, 3, BITSIZE - 3, CountBitsByteTable<uint16_t>, "uint16_t count bits ByteTable");
    test_countbits( data16unsigned, 3, BITSIZE - 3, CountBitsByteDirect<uint16_t>, "uint16_t count bits ByteDirect");
    test_countbits( data16unsigned, 3, BITSIZE - 3, CountBitsByteDirect2<uint16_t>, "uint16_t count bits ByteDirect2");
    test_countbits( data16unsigned, 3, BITSIZE - 3, CountBitsByteLoop<uint16_t>, "uint16_t count bits ByteLoop");
    test_countbits( data16unsigned, 3, BITSIZE - 3, CountBitsByteLoop2<uint16_t>, "uint16_t count bits ByteLoop2");
    test_countbits( data16unsigned, 3, BITSIZE - 3, CountBitsShift<uint16_t>, "uint16_t count bits shift");
    test_countbits( data16unsigned, 3, BITSIZE - 3, CountBits<uint16_t>, "uint16_t count bits");
    
    summarize("uint16_t bitarrays", BITSIZE, iterations, kDontShowGMeans, kDontShowPenalty );
    
    
// uint32_t
    size_t setCount32 = (BITSIZE+31)/(8*sizeof(uint32_t));
    ::fill(data32unsigned, data32unsigned+setCount32, uint32_t(init_value));
    size_t valueExpected32 = CountBitsShift( data32unsigned, 3, BITSIZE-3 );
    ::fill(data32unsigned2, data32unsigned2+setCount32, uint32_t(init_value));
    ::fill(data32unsigned3, data32unsigned3+setCount32, uint32_t(0));
    size_t invertExpected32 = valueExpected32;
    
    test_setbits( data32unsigned, 3, BITSIZE - 3, (BITSIZE-6), SetBitsOptimized<uint32_t>, "uint32_t setbits optimized");
    test_setbits( data32unsigned, 3, BITSIZE - 3, (BITSIZE-6), SetBitsHalfOpt<uint32_t>, "uint32_t setbits half-opt");
    test_setbits( data32unsigned, 3, BITSIZE - 3, (BITSIZE-6), SetBitsShift<uint32_t>, "uint32_t setbits shift");
    test_setbits( data32unsigned, 3, BITSIZE - 3, (BITSIZE-6), SetBits<uint32_t>, "uint32_t setbits");
    
    test_setbits( data32unsigned, 3, BITSIZE - 3, 0, ClearBitsHalfOpt<uint32_t>, "uint32_t clearbits half-opt");
    test_setbits( data32unsigned, 3, BITSIZE - 3, 0, ClearBitsShift<uint32_t>, "uint32_t clearbits shift");
    test_setbits( data32unsigned, 3, BITSIZE - 3, 0, ClearBits<uint32_t>, "uint32_t clearbits");
    
    test_setbits( data32unsigned2, 3, BITSIZE - 3, invertExpected32, InvertBitsHalfOpt<uint32_t>, "uint32_t invertbits half-opt");
    test_setbits( data32unsigned2, 3, BITSIZE - 3, invertExpected32, InvertBitsShift<uint32_t>, "uint32_t invertbits shift");
    test_setbits( data32unsigned2, 3, BITSIZE - 3, invertExpected32, InvertBits<uint32_t>, "uint32_t invertbits");
    
    ::fill(data32unsigned, data32unsigned+setCount32, uint32_t(init_value));
    ::fill(data32unsigned2, data32unsigned2+setCount32, uint32_t(init_value));
    test_mergebits( data32unsigned, data32unsigned2, 3, BITSIZE - 3, valueExpected32, AndBitTablesOptimized<uint32_t>, "uint32_t and bit tables optimized");
    test_mergebits( data32unsigned, data32unsigned2, 3, BITSIZE - 3, valueExpected32, AndBitTablesHalfOpt<uint32_t>, "uint32_t and bit tables half-opt");
    test_mergebits( data32unsigned, data32unsigned2, 3, BITSIZE - 3, valueExpected32, AndBitTablesShift<uint32_t>, "uint32_t and bit tables shift");
    test_mergebits( data32unsigned, data32unsigned2, 3, BITSIZE - 3, valueExpected32, AndBitTables<uint32_t>, "uint32_t and bit tables");
    
    test_mergebits( data32unsigned, data32unsigned2, 3, BITSIZE - 3, valueExpected32, OrBitTablesHalfOpt<uint32_t>, "uint32_t or bit tables half-opt");
    test_mergebits( data32unsigned, data32unsigned2, 3, BITSIZE - 3, valueExpected32, OrBitTablesShift<uint32_t>, "uint32_t or bit tables shift");
    test_mergebits( data32unsigned, data32unsigned2, 3, BITSIZE - 3, valueExpected32, OrBitTables<uint32_t>, "uint32_t or bit tables");
    
    test_mergebits( data32unsigned, data32unsigned2, 3, BITSIZE - 3, invertExpected32, XorBitTablesHalfOpt<uint32_t>, "uint32_t xor bit tables half-opt");
    test_mergebits( data32unsigned, data32unsigned2, 3, BITSIZE - 3, invertExpected32, XorBitTablesShift<uint32_t>, "uint32_t xor bit tables shift");
    test_mergebits( data32unsigned, data32unsigned2, 3, BITSIZE - 3, invertExpected32, XorBitTables<uint32_t>, "uint32_t xor bit tables");

    test_mergebits( data32unsigned, data32unsigned2, 3, BITSIZE - 3, 0, AndComplimentBitTablesHalfOpt<uint32_t>, "uint32_t and compliment bit tables half-opt");
    test_mergebits( data32unsigned, data32unsigned2, 3, BITSIZE - 3, 0, AndComplimentBitTablesShift<uint32_t>, "uint32_t and compliment bit tables shift");
    test_mergebits( data32unsigned, data32unsigned2, 3, BITSIZE - 3, 0, AndComplimentBitTables<uint32_t>, "uint32_t and compliment bit tables");

    ::fill(data32unsigned, data32unsigned+setCount32, uint32_t(init_value));
    ::fill(data32unsigned2, data32unsigned2+setCount32, uint32_t(init_value));
    test_blitbits( data32unsigned3, data32unsigned2, data32unsigned, 3, BITSIZE - 3, valueExpected32, StencilBitTablesHalfOpt<uint32_t>, "uint32_t stencil bit tables half-opt");
    test_blitbits( data32unsigned3, data32unsigned2, data32unsigned, 3, BITSIZE - 3, valueExpected32, StencilBitTablesShift<uint32_t>, "uint32_t stencil bit tables shift");
    test_blitbits( data32unsigned3, data32unsigned2, data32unsigned, 3, BITSIZE - 3, valueExpected32, StencilBitTables<uint32_t>, "uint32_t stencil bit tables");

    SetBitsOptimized( data32unsigned, 3, BITSIZE- 3);
    test_countbits( data32unsigned, 3, BITSIZE - 3, CountBits32Multiply<uint32_t>, "uint32_t count bits 32Multiply");
    test_countbits( data32unsigned, 3, BITSIZE - 3, CountBits32Parallel<uint32_t>, "uint32_t count bits 32Parallel");
    test_countbits( data32unsigned, 3, BITSIZE - 3, CountBitsByteTable<uint32_t>, "uint32_t count bits ByteTable");
    test_countbits( data32unsigned, 3, BITSIZE - 3, CountBitsByteDirect<uint32_t>, "uint32_t count bits ByteDirect");
    test_countbits( data32unsigned, 3, BITSIZE - 3, CountBitsByteDirect2<uint32_t>, "uint32_t count bits ByteDirect2");
    test_countbits( data32unsigned, 3, BITSIZE - 3, CountBitsByteLoop<uint32_t>, "uint32_t count bits ByteLoop");
    test_countbits( data32unsigned, 3, BITSIZE - 3, CountBitsByteLoop2<uint32_t>, "uint32_t count bits ByteLoop2");
    test_countbits( data32unsigned, 3, BITSIZE - 3, CountBitsShift<uint32_t>, "uint32_t count bits shift");
    test_countbits( data32unsigned, 3, BITSIZE - 3, CountBits<uint32_t>, "uint32_t count bits");
    
    summarize("uint32_t bitarrays", BITSIZE, iterations, kDontShowGMeans, kDontShowPenalty );
    
    
// uint64_t
    size_t setCount64 = (BITSIZE+63)/(8*sizeof(uint64_t));
    ::fill(data64unsigned, data64unsigned+setCount64, uint64_t(init_value));
    size_t valueExpected64 = CountBitsShift( data64unsigned, 3, BITSIZE-3 );
    ::fill(data64unsigned2, data64unsigned2+setCount64, uint64_t(init_value));
    ::fill(data64unsigned3, data64unsigned3+setCount64, uint64_t(0));
    size_t invertExpected64 = valueExpected64;
    
    test_setbits( data64unsigned, 3, BITSIZE - 3, (BITSIZE-6), SetBitsOptimized<uint64_t>, "uint64_t setbits optimized");
    test_setbits( data64unsigned, 3, BITSIZE - 3, (BITSIZE-6), SetBitsHalfOpt<uint64_t>, "uint64_t setbits half-opt");
    test_setbits( data64unsigned, 3, BITSIZE - 3, (BITSIZE-6), SetBitsShift<uint64_t>, "uint64_t setbits shift");
    test_setbits( data64unsigned, 3, BITSIZE - 3, (BITSIZE-6), SetBits<uint64_t>, "uint64_t setbits");
    
    test_setbits( data64unsigned, 3, BITSIZE - 3, 0, ClearBitsHalfOpt<uint64_t>, "uint64_t clearbits half-opt");
    test_setbits( data64unsigned, 3, BITSIZE - 3, 0, ClearBitsShift<uint64_t>, "uint64_t clearbits shift");
    test_setbits( data64unsigned, 3, BITSIZE - 3, 0, ClearBits<uint64_t>, "uint64_t clearbits");
    
    test_setbits( data64unsigned2, 3, BITSIZE - 3, invertExpected64, InvertBitsHalfOpt<uint64_t>, "uint64_t invertbits half-opt");
    test_setbits( data64unsigned2, 3, BITSIZE - 3, invertExpected64, InvertBitsShift<uint64_t>, "uint64_t invertbits shift");
    test_setbits( data64unsigned2, 3, BITSIZE - 3, invertExpected64, InvertBits<uint64_t>, "uint64_t invertbits");
    
    ::fill(data64unsigned, data64unsigned+setCount64, uint64_t(init_value));
    ::fill(data64unsigned2, data64unsigned2+setCount64, uint64_t(init_value));
    test_mergebits( data64unsigned, data64unsigned2, 3, BITSIZE - 3, valueExpected64, AndBitTablesOptimized<uint64_t>, "uint64_t and bit tables optimized");
    test_mergebits( data64unsigned, data64unsigned2, 3, BITSIZE - 3, valueExpected64, AndBitTablesHalfOpt<uint64_t>, "uint64_t and bit tables half-opt");
    test_mergebits( data64unsigned, data64unsigned2, 3, BITSIZE - 3, valueExpected64, AndBitTablesShift<uint64_t>, "uint64_t and bit tables shift");
    test_mergebits( data64unsigned, data64unsigned2, 3, BITSIZE - 3, valueExpected64, AndBitTables<uint64_t>, "uint64_t and bit tables");
    
    test_mergebits( data64unsigned, data64unsigned2, 3, BITSIZE - 3, valueExpected64, OrBitTablesHalfOpt<uint64_t>, "uint64_t or bit tables half-opt");
    test_mergebits( data64unsigned, data64unsigned2, 3, BITSIZE - 3, valueExpected64, OrBitTablesShift<uint64_t>, "uint64_t or bit tables shift");
    test_mergebits( data64unsigned, data64unsigned2, 3, BITSIZE - 3, valueExpected64, OrBitTables<uint64_t>, "uint64_t or bit tables");
    
    test_mergebits( data64unsigned, data64unsigned2, 3, BITSIZE - 3, invertExpected64, XorBitTablesHalfOpt<uint64_t>, "uint64_t xor bit tables half-opt");
    test_mergebits( data64unsigned, data64unsigned2, 3, BITSIZE - 3, invertExpected64, XorBitTablesShift<uint64_t>, "uint64_t xor bit tables shift");
    test_mergebits( data64unsigned, data64unsigned2, 3, BITSIZE - 3, invertExpected64, XorBitTables<uint64_t>, "uint64_t xor bit tables");

    test_mergebits( data64unsigned, data64unsigned2, 3, BITSIZE - 3, 0, AndComplimentBitTablesHalfOpt<uint64_t>, "uint64_t and compliment bit tables half-opt");
    test_mergebits( data64unsigned, data64unsigned2, 3, BITSIZE - 3, 0, AndComplimentBitTablesShift<uint64_t>, "uint64_t and compliment bit tables shift");
    test_mergebits( data64unsigned, data64unsigned2, 3, BITSIZE - 3, 0, AndComplimentBitTables<uint64_t>, "uint64_t and compliment bit tables");

    ::fill(data64unsigned, data64unsigned+setCount64, uint64_t(init_value));
    ::fill(data64unsigned2, data64unsigned2+setCount64, uint64_t(init_value));
    test_blitbits( data64unsigned3, data64unsigned2, data64unsigned, 3, BITSIZE - 3, valueExpected64, StencilBitTablesHalfOpt<uint64_t>, "uint64_t stencil bit tables half-opt");
    test_blitbits( data64unsigned3, data64unsigned2, data64unsigned, 3, BITSIZE - 3, valueExpected64, StencilBitTablesShift<uint64_t>, "uint64_t stencil bit tables shift");
    test_blitbits( data64unsigned3, data64unsigned2, data64unsigned, 3, BITSIZE - 3, valueExpected64, StencilBitTables<uint64_t>, "uint64_t stencil bit tables");

    SetBitsOptimized( data64unsigned, 3, BITSIZE- 3);
    test_countbits( data64unsigned, 3, BITSIZE - 3, CountBits32Multiply<uint64_t>, "uint64_t count bits 32Multiply");
    test_countbits( data64unsigned, 3, BITSIZE - 3, CountBits32Parallel<uint64_t>, "uint64_t count bits 32Parallel");
    test_countbits( data64unsigned, 3, BITSIZE - 3, CountBitsByteTable<uint64_t>, "uint64_t count bits ByteTable");
    test_countbits( data64unsigned, 3, BITSIZE - 3, CountBitsByteDirect<uint64_t>, "uint64_t count bits ByteDirect");
    test_countbits( data64unsigned, 3, BITSIZE - 3, CountBitsByteDirect2<uint64_t>, "uint64_t count bits ByteDirect2");
    test_countbits( data64unsigned, 3, BITSIZE - 3, CountBitsByteLoop<uint64_t>, "uint64_t count bits ByteLoop");
    test_countbits( data64unsigned, 3, BITSIZE - 3, CountBitsByteLoop2<uint64_t>, "uint64_t count bits ByteLoop2");
    test_countbits( data64unsigned, 3, BITSIZE - 3, CountBitsShift<uint64_t>, "uint64_t count bits shift");
    test_countbits( data64unsigned, 3, BITSIZE - 3, CountBits<uint64_t>, "uint64_t count bits");

    summarize("uint64_t bitarrays", BITSIZE, iterations, kDontShowGMeans, kDontShowPenalty );



// std::bitset
    testBitset dataStd;
    testBitset dataStd2;
    testBitset dataStd3;
    
    dataStd.set();
    dataStd2.set();
    dataStd3.reset();
    size_t valueExpectedStd = dataStd.count();

    test_setbitsStd( dataStd, valueExpectedStd, SetBitsStd, "std bitset setbits");
    test_setbitsStd( dataStd, valueExpectedStd, SetBitsStdLoop, "std bitset setbits loop");
    test_setbitsStd( dataStd, valueExpectedStd, SetBitsStdLoop, "std bitset setbits loop2");
    test_setbitsStd( dataStd, 0, ClearBitsStd, "std bitset clearbits");
    test_setbitsStd( dataStd, 0, ClearBitsStdLoop, "std bitset clearbits loop");
    test_setbitsStd( dataStd, 0, ClearBitsStdLoop, "std bitset clearbits loop2");
    test_setbitsStd( dataStd2, valueExpectedStd, InvertBitsStd, "std bitset invertbits");
    test_setbitsStd( dataStd2, valueExpectedStd, InvertBitsStdLoop, "std bitset invertbits loop");
    test_setbitsStd( dataStd2, valueExpectedStd, InvertBitsStdLoop, "std bitset invertbits loop2");

    dataStd.set();
    dataStd2.set();
    test_mergebitsStd( dataStd, dataStd2, valueExpectedStd, AndBitTablesStd, "std bitset and bit tables");
    test_mergebitsStd( dataStd, dataStd2, valueExpectedStd, AndBitTablesStdLoop, "std bitset and bit tables loop");
    test_mergebitsStd( dataStd, dataStd2, valueExpectedStd, AndBitTablesStdLoop2, "std bitset and bit tables loop2");
    test_mergebitsStd( dataStd, dataStd2, valueExpectedStd, OrBitTablesStd, "std bitset or bit tables");
    test_mergebitsStd( dataStd, dataStd2, valueExpectedStd, OrBitTablesStdLoop, "std bitset or bit tables loop");
    test_mergebitsStd( dataStd, dataStd2, valueExpectedStd, OrBitTablesStdLoop2, "std bitset or bit tables loop2");
    test_mergebitsStd( dataStd, dataStd2, valueExpectedStd, XorBitTablesStd, "std bitset xor bit tables");
    test_mergebitsStd( dataStd, dataStd2, valueExpectedStd, XorBitTablesStdLoop, "std bitset xor bit tables loop");
    test_mergebitsStd( dataStd, dataStd2, 0, AndComplimentBitTablesStd, "std bitset and compliment bit tables");
    test_mergebitsStd( dataStd, dataStd2, 0, AndComplimentBitTablesStdLoop, "std bitset and compliment bit tables loop");
    test_mergebitsStd( dataStd, dataStd2, 0, AndComplimentBitTablesStdLoop2, "std bitset and compliment bit tables loop2");

    dataStd.set();
    dataStd2.set();
    test_blitbitsStd( dataStd3, dataStd2, dataStd, valueExpectedStd, StencilBitTablesStd, "std bitset stencil bit tables");
    test_blitbitsStd( dataStd3, dataStd2, dataStd, valueExpectedStd, StencilBitTablesStdLoop, "std bitset stencil bit tables loop");
    test_blitbitsStd( dataStd3, dataStd2, dataStd, valueExpectedStd, StencilBitTablesStdLoop2, "std bitset stencil bit tables loop2");

    dataStd.set();
    test_countbitsStd( dataStd, "std bitset count bits");
    test_countbitsStdLoop( dataStd, "std bitset count bits loop");

    summarize("std bitset", BITSIZE, iterations, kDontShowGMeans, kDontShowPenalty );


            
    return 0;
}

// the end
/******************************************************************************/
/******************************************************************************/
